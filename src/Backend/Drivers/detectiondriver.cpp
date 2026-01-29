#include "detectiondriver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

DetectionDriver::DetectionDriver(QObject *parent) : QObject(parent)
{
    m_tcpServer = new QTcpServer(this);
    m_currentClient = nullptr;

    // 监听新连接
    connect(m_tcpServer, &QTcpServer::newConnection, this, &DetectionDriver::onNewConnection);

    // 数据超时看门狗 (3秒无数据则清空界面)
    m_dataExpiryTimer = new QTimer(this);
    m_dataExpiryTimer->setInterval(3000);
    connect(m_dataExpiryTimer, &QTimer::timeout, this, &DetectionDriver::onDataTimeout);
}

DetectionDriver::~DetectionDriver()
{
    if (m_currentClient) {
        m_currentClient->close();
    }
    m_tcpServer->close();
}

void DetectionDriver::startServer(int port)
{
    // 监听所有网卡 (0.0.0.0)，端口通常是 8089
    if (m_tcpServer->listen(QHostAddress::Any, port)) {
        qDebug() << "[Detection] TCP Server 启动成功，监听端口:" << port;
    } else {
        qCritical() << "[Detection] TCP Server 启动失败:" << m_tcpServer->errorString();
    }
}

// =========================================================
// 连接管理
// =========================================================

void DetectionDriver::onNewConnection()
{
    if (m_currentClient) {
        // 如果已经有一个连接，把旧的踢掉 (只允许一个设备连接)
        m_currentClient->close();
        m_currentClient->deleteLater();
    }

    m_currentClient = m_tcpServer->nextPendingConnection();
    qDebug() << "[Detection] Linux 设备已连接:" << m_currentClient->peerAddress().toString();

    connect(m_currentClient, &QTcpSocket::readyRead, this, &DetectionDriver::onReadyRead);
    connect(m_currentClient, &QTcpSocket::disconnected, this, &DetectionDriver::onSocketDisconnected);

    // 连接建立，启动看门狗
    m_dataExpiryTimer->start();
    m_buffer.clear();
}

void DetectionDriver::onSocketDisconnected()
{
    qWarning() << "[Detection] 设备断开连接";
    m_currentClient->deleteLater();
    m_currentClient = nullptr;
    onDataTimeout(); // 立即清空数据
}

void DetectionDriver::onDataTimeout()
{
    // 没收到数据，清空界面
    // qDebug() << "[Detection] 数据流超时/断开，清空界面";
    clearAllData();
}

void DetectionDriver::clearAllData()
{
    emit sigDroneListUpdated({});
    emit sigImageListUpdated({});
    emit sigAlertCountUpdated(0);
}

// =========================================================
// 数据接收与粘包处理 (核心)
// =========================================================

void DetectionDriver::onReadyRead()
{
    if (!m_currentClient) return;

    // 1. 读取所有新数据追加到缓冲区
    QByteArray newData = m_currentClient->readAll();
    m_buffer.append(newData);

    // 2. 喂狗 (重置超时时间)
    m_dataExpiryTimer->start();

    // 3. 处理缓冲区
    processBuffer();
}

void DetectionDriver::processBuffer()
{
    // 协议格式: 55 55 55 55 ... { JSON } ... AA AA AA AA
    // 我们采用最简单的“寻找 JSON 边界”的方法，不依赖头部的长度字段(防止字节序问题)

    while (true) {
        // A. 找 JSON 起始符 '{'
        int startIdx = m_buffer.indexOf('{');
        if (startIdx == -1) {
            // 没有起始符，清空之前的垃圾数据，保留最后一部分防止截断(或者直接全清空如果buffer太大)
            if (m_buffer.size() > 8192) m_buffer.clear();
            break;
        }

        // B. 找 JSON 结束符 '}'
        // 注意：简单的找 '}' 可能会因为 JSON 嵌套而出错，
        // 但根据你的日志，数据包是一个完整的 JSON 对象，我们找最后一个匹配的 '}'
        // 这里简化处理：寻找对应 startIdx 之后的第一个 '}' 是不够的(有嵌套)，
        // 我们利用栈或者简单找 Buffer 里的 '}' 尝试解析。

        // 优化策略：找到 headers "55 55 55 55" 和 tails "AA AA AA AA"
        // 既然协议有头尾，用头尾更稳

        // 1. 找头
        int headIdx = m_buffer.indexOf(QByteArray::fromHex("55555555"));
        if (headIdx == -1) {
            if (m_buffer.size() > 8192) m_buffer.clear();
            break;
        }

        // 2. 找尾 (从头后面开始找)
        int tailIdx = m_buffer.indexOf(QByteArray::fromHex("AAAAAAAA"), headIdx);
        if (tailIdx == -1) {
            // 包不完整，等待下次数据
            break;
        }

        // 3. 提取完整一包数据
        // 包总长 = (tailIdx + 4) - headIdx
        int packetLen = (tailIdx + 4) - headIdx;
        QByteArray packet = m_buffer.mid(headIdx, packetLen);

        // 4. 从包中提取 JSON 字符串
        int jsonStart = packet.indexOf('{');
        int jsonEnd = packet.lastIndexOf('}');

        if (jsonStart != -1 && jsonEnd != -1 && jsonEnd > jsonStart) {
            QByteArray jsonBytes = packet.mid(jsonStart, jsonEnd - jsonStart + 1);
            parseJsonData(jsonBytes);
        }

        // 5. 从缓冲区移除已处理的数据
        m_buffer.remove(0, headIdx + packetLen);
    }
}

// =========================================================
// JSON 业务解析
// =========================================================

void DetectionDriver::parseJsonData(const QByteArray &jsonBytes)
{
    QJsonDocument doc = QJsonDocument::fromJson(jsonBytes);
    if (!doc.isObject()) return;

    QJsonObject root = doc.object();

    // 1. 无人机信息
    if (root.contains("station_droneInfo")) {
        handleDroneInfo(root["station_droneInfo"].toObject());
    }
    // 2. 图传/频谱
    else if (root.contains("imageInfo")) {
        handleImageInfo(root["imageInfo"].toObject());
    }
    // 3. FPV
    else if (root.contains("fpvInfo")) {
        handleFpvInfo(root["fpvInfo"].toObject());
    }
    // 4. 基站状态 (包含自身坐标)
    else if (root.contains("device_status") || root.contains("station_pos")) {
        handleDeviceStatus(root);
    }
}

void DetectionDriver::handleDroneInfo(const QJsonObject &data)
{
    // data 是 station_droneInfo 层
    QJsonObject trace = data["trace"].toObject();
    if (trace.isEmpty()) return;

    QList<DroneInfo> list;
    DroneInfo d;
    d.uav_id     = trace["uav_id"].toString();
    d.model_name = trace["model_name"].toString();

    // 注意：日志里的坐标可能是字符串 "0.0" 也可能是数字，这里做个兼容
    if (trace["uav_lat"].isString()) d.uav_lat = trace["uav_lat"].toString().toDouble();
    else d.uav_lat = trace["uav_lat"].toDouble();

    if (trace["uav_lng"].isString()) d.uav_lng = trace["uav_lng"].toString().toDouble();
    else d.uav_lng = trace["uav_lng"].toDouble();

    d.height = trace["Height"].toDouble(); // 注意大小写，日志是 Height
    if (d.height == 0) d.height = trace["height"].toDouble(); // 兼容小写

    d.freq = trace["freq"].toDouble();
    d.pilot_lat = trace["pilot_lat"].toDouble();
    d.pilot_lng = trace["pilot_lng"].toDouble();

    // 如果坐标全是 0 或 None，可以给一个特定的状态，但这里直接发给 UI
    list.append(d);

    emit sigDroneListUpdated(list);
    emit sigAlertCountUpdated(list.size());
}

void DetectionDriver::handleImageInfo(const QJsonObject &data)
{
    // 构造 ImageInfo
    QList<ImageInfo> list;
    ImageInfo img;
    img.id = "Spectrum_" + QString::number(data["freq"].toDouble());
    img.freq = data["freq"].toDouble();
    img.amplitude = data["amplitude"].toDouble();
    img.type = 0; // 0 代表图传/频谱
    // img.mes = ... 时间戳
    list.append(img);

    emit sigImageListUpdated(list);
}

void DetectionDriver::handleFpvInfo(const QJsonObject &data)
{
    QList<ImageInfo> list;
    ImageInfo img;
    img.id = "FPV_" + QString::number(data["freq"].toDouble());
    img.freq = data["freq"].toDouble();
    img.amplitude = data["amplitude"].toDouble();
    img.type = 1; // 1 代表 FPV
    list.append(img);

    emit sigImageListUpdated(list);
}

void DetectionDriver::handleDeviceStatus(const QJsonObject &root)
{
    // 解析基站坐标 (用于更新地图中心点)
    // 结构可能是 root["station_pos"] -> {lat, lng}
    if (root.contains("station_pos")) {
        QJsonObject pos = root["station_pos"].toObject();
        double lat = pos["lat"].toDouble();
        double lng = pos["lng"].toDouble();
        if (lat > 0 && lng > 0) {
            emit sigDevicePositionUpdated(lat, lng);
        }
    }
}
