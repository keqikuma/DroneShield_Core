#include "detectiondriver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QCoreApplication>

DetectionDriver::DetectionDriver(QObject *parent) : QObject(parent)
{
    m_tcpServer = new QTcpServer(this);
    m_currentClient = nullptr;

    connect(m_tcpServer, &QTcpServer::newConnection, this, &DetectionDriver::onNewConnection);

    m_dataExpiryTimer = new QTimer(this);
    m_dataExpiryTimer->setInterval(3000);
    connect(m_dataExpiryTimer, &QTimer::timeout, this, &DetectionDriver::onDataTimeout);
}

DetectionDriver::~DetectionDriver()
{
    if (m_currentClient) m_currentClient->close();
    m_tcpServer->close();
}

void DetectionDriver::writeLog(const QString &msg)
{
    // 将日志写入 exe 同级目录下的 log.txt
    QString path = QCoreApplication::applicationDirPath() + "/app_debug_log.txt";
    QFile file(path);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << "[" << QDateTime::currentDateTime().toString("HH:mm:ss.zzz") << "] " << msg << "\n";
        file.close();
    }
    // 同时发信号给界面显示
    emit sigLogMessage(msg);
}

void DetectionDriver::startServer(int port)
{
    if (m_tcpServer->listen(QHostAddress::Any, port)) {
        writeLog(QString("✅ TCP Server 启动成功，监听端口: %1").arg(port));
    } else {
        writeLog(QString("❌ TCP Server 启动失败! 错误: %1").arg(m_tcpServer->errorString()));
    }
}

void DetectionDriver::onNewConnection()
{
    if (m_currentClient) {
        m_currentClient->close();
        m_currentClient->deleteLater();
    }

    m_currentClient = m_tcpServer->nextPendingConnection();
    QString ip = m_currentClient->peerAddress().toString();
    writeLog(QString("🔗 新客户端连接: %1").arg(ip));

    connect(m_currentClient, &QTcpSocket::readyRead, this, &DetectionDriver::onReadyRead);
    connect(m_currentClient, &QTcpSocket::disconnected, this, &DetectionDriver::onSocketDisconnected);

    m_dataExpiryTimer->start();
    m_buffer.clear();
}

void DetectionDriver::onSocketDisconnected()
{
    writeLog("⚠️ 客户端断开连接");
    m_currentClient->deleteLater();
    m_currentClient = nullptr;
    onDataTimeout();
}

void DetectionDriver::onDataTimeout()
{
    clearAllData();
}

void DetectionDriver::clearAllData()
{
    emit sigDroneListUpdated({});
    emit sigImageListUpdated({});
    emit sigAlertCountUpdated(0);
}

void DetectionDriver::onReadyRead()
{
    if (!m_currentClient) return;
    QByteArray newData = m_currentClient->readAll();

    // 简单日志，证明数据进来了
    if (!newData.isEmpty()) {
        writeLog(QString("📥 收到数据: %1 字节").arg(newData.size()));
    }

    m_buffer.append(newData);
    m_dataExpiryTimer->start();
    processBuffer();
}

// ====================================================================
// 【核心修改】通用 JSON 提取算法 (花括号计数法)
// ====================================================================
void DetectionDriver::processBuffer()
{
    // 循环处理缓冲区，直到没有完整的 JSON 为止
    while (true) {
        int startIdx = -1;
        int endIdx = -1;
        int braceCount = 0;
        bool foundCompleteJson = false;

        // 1. 扫描缓冲区，寻找完整的 {...} 结构
        for (int i = 0; i < m_buffer.size(); ++i) {
            char c = m_buffer.at(i);

            if (c == '{') {
                if (braceCount == 0) startIdx = i; // 记录最外层左括号
                braceCount++;
            }
            else if (c == '}') {
                if (braceCount > 0) {
                    braceCount--;
                    if (braceCount == 0) {
                        // 找到了匹配的最外层右括号
                        endIdx = i;
                        foundCompleteJson = true;
                        break; // 跳出 for 循环，处理这一段
                    }
                }
            }
        }

        // 2. 判断是否找到
        if (foundCompleteJson && startIdx != -1 && endIdx != -1) {
            // 提取 JSON 字符串
            int jsonLen = endIdx - startIdx + 1;
            QByteArray jsonBytes = m_buffer.mid(startIdx, jsonLen);

            // 调试日志：看看提取到了什么 (只打印前50个字符避免刷屏)
            // writeLog(QString("📝 提取 JSON: %1...").arg(QString(jsonBytes.left(50))));

            // 解析
            parseJsonData(jsonBytes);

            // 关键：从缓冲区移除已处理的数据（包括 endIdx 及其之前的所有内容）
            m_buffer.remove(0, endIdx + 1);
        } else {
            // 没找到完整的 JSON，或者数据还没收全
            // 清理缓冲区头部垃圾：如果 buffer 开头不是 '{' 且长度很大，说明前面是乱码/协议头
            int firstBrace = m_buffer.indexOf('{');
            if (firstBrace > 0) {
                // writeLog(QString("🗑️ 丢弃头部非 JSON 数据: %1 字节").arg(firstBrace));
                m_buffer.remove(0, firstBrace);
                continue; // 重新扫描
            }

            // 如果缓冲区太大还没找到 JSON，强制清空防止内存泄漏
            if (m_buffer.size() > 100000) {
                writeLog("❌ 缓冲区溢出，强制清空");
                m_buffer.clear();
            }

            break; // 等待下一次 readyRead
        }
    }
}

void DetectionDriver::parseJsonData(const QByteArray &jsonBytes)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &err);

    if (err.error != QJsonParseError::NoError) {
        writeLog(QString("❌ JSON 格式错误: %1").arg(err.errorString()));
        return;
    }

    if (!doc.isObject()) return;
    QJsonObject root = doc.object();

    // 调试：打印所有收到的 Key，帮你看清到底是什么名字
    QString keys = root.keys().join(", ");
    writeLog(QString("🔑 收到 Keys: %1").arg(keys));

    // 根据 Python 脚本的验证结果，Key 应该是下面这些：
    if (root.contains("station_droneInfo")) {
        writeLog("✅ 解析: 无人机信息");
        handleDroneInfo(root["station_droneInfo"].toObject());
    }
    else if (root.contains("imageInfo")) {
        writeLog("✅ 解析: 图传/频谱");
        handleImageInfo(root["imageInfo"].toObject());
    }
    else if (root.contains("fpvInfo")) {
        writeLog("✅ 解析: FPV");
        handleFpvInfo(root["fpvInfo"].toObject());
    }
    else if (root.contains("device_status") || root.contains("station_pos")) {
        writeLog("❤️ 解析: 基站状态");
        handleDeviceStatus(root);
    }
    else {
        writeLog("❓ 未知数据包，包含 Keys: " + root.keys().join(", "));
    }
}

void DetectionDriver::handleDroneInfo(const QJsonObject &data)
{
    QJsonObject trace = data["trace"].toObject();
    if (trace.isEmpty()) return;

    QList<DroneInfo> list;
    DroneInfo d;
    d.uav_id = trace["uav_id"].toString();
    d.model_name = trace["model_name"].toString();

    // 坐标兼容
    if (trace["uav_lat"].isString()) d.uav_lat = trace["uav_lat"].toString().toDouble();
    else d.uav_lat = trace["uav_lat"].toDouble();

    if (trace["uav_lng"].isString()) d.uav_lng = trace["uav_lng"].toString().toDouble();
    else d.uav_lng = trace["uav_lng"].toDouble();

    // 高度兼容 (Height vs height)
    if (trace.contains("Height")) d.height = trace["Height"].toDouble();
    else if (trace.contains("height")) d.height = trace["height"].toDouble();
    else d.height = 0;

    d.freq = trace["freq"].toDouble();
    d.pilot_lat = trace["pilot_lat"].toDouble();
    d.pilot_lng = trace["pilot_lng"].toDouble();
    d.distance = trace["distance"].toDouble();
    d.uuid = trace["uuid"].toString();
    d.azimuth = trace["azimuth"].toDouble(); // 确保有方位角

    // writeLog(QString(">>> 更新无人机: %1 (ID: %2)").arg(d.model_name).arg(d.uav_id));

    list.append(d);
    emit sigDroneListUpdated(list);
    emit sigAlertCountUpdated(list.size());
}

void DetectionDriver::handleImageInfo(const QJsonObject &data)
{
    QList<ImageInfo> list;
    ImageInfo img;
    // 使用频率作为唯一 ID 显示
    img.id = "Spectrum " + QString::number(data["freq"].toDouble()) + "MHz";
    img.freq = data["freq"].toDouble();
    img.amplitude = data["amplitude"].toDouble();
    img.type = 0; // 图传

    // 如果有协议字段
    if (data.contains("pro")) {
        img.id += " (" + data["pro"].toString() + ")";
    }

    list.append(img);
    emit sigImageListUpdated(list);
}

void DetectionDriver::handleFpvInfo(const QJsonObject &data)
{
    QList<ImageInfo> list;
    ImageInfo img;
    img.id = "FPV " + QString::number(data["freq"].toDouble()) + "MHz";
    img.freq = data["freq"].toDouble();
    img.amplitude = data["amplitude"].toDouble();
    img.type = 1; // FPV
    list.append(img);
    emit sigImageListUpdated(list);
}

void DetectionDriver::handleDeviceStatus(const QJsonObject &root)
{
    if (root.contains("station_pos")) {
        QJsonObject pos = root["station_pos"].toObject();
        double lat = pos["lat"].toDouble();
        double lng = pos["lng"].toDouble();
        if (lat > 0.1 && lng > 0.1) {
            emit sigDevicePositionUpdated(lat, lng);
        }
    }
}
