#include "detectiondriver.h"
#include <QDebug>

DetectionDriver::DetectionDriver(QObject *parent) : QObject(parent)
{
    m_webSocket = new QWebSocket();

    connect(m_webSocket, &QWebSocket::connected, this, &DetectionDriver::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &DetectionDriver::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &DetectionDriver::onTextMessageReceived);

    // 重连定时器
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(5000);
    connect(m_reconnectTimer, &QTimer::timeout, this, &DetectionDriver::onReconnectTimeout);

    // 【新增】心跳定时器
    m_heartbeatTimer = new QTimer(this);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &DetectionDriver::onHeartbeatTimeout);
}

DetectionDriver::~DetectionDriver()
{
    m_webSocket->close();
}

void DetectionDriver::startWork(const QString &url)
{
    m_targetUrl = url;
    m_webSocket->open(QUrl(m_targetUrl));
    emit sigLog("[侦测] 正在连接 WebSocket...");
}

void DetectionDriver::stopWork()
{
    m_reconnectTimer->stop();
    m_heartbeatTimer->stop(); // 停止心跳
    m_webSocket->close();
}

void DetectionDriver::onConnected()
{
    emit sigLog("[侦测] WebSocket 已连接");
    m_reconnectTimer->stop();
    // 注意：心跳定时器在收到握手包(0...)后才启动
}

void DetectionDriver::onDisconnected()
{
    emit sigLog("[侦测] WebSocket 断开，5秒后重连...");
    m_heartbeatTimer->stop(); // 断开时必须停止心跳
    m_reconnectTimer->start();

    // 断开时，发送空列表清空 UI，避免显示过时数据
    emit sigDroneListUpdated(QList<DroneInfo>());
}

void DetectionDriver::onReconnectTimeout()
{
    m_webSocket->open(QUrl(m_targetUrl));
}

// 【新增】心跳发送
void DetectionDriver::onHeartbeatTimeout()
{
    if (m_webSocket->isValid()) {
        // Socket.IO 心跳包就是简单的字符串 "2"
        m_webSocket->sendTextMessage("2");
        // qDebug() << "[侦测] 发送心跳: 2";
    }
}

// ============================================================================
// 核心：消息解析
// ============================================================================
void DetectionDriver::onTextMessageReceived(const QString &message)
{
    // 1. 处理握手包 (0...)
    // 格式: 0{"sid":"...","pingInterval":25000,"pingTimeout":20000}
    if (message.startsWith("0")) {
        handleHandshake(message.mid(1));
        return;
    }

    // 2. 处理服务器的心跳回执 (3)
    if (message == "3") {
        // 收到 Pong，连接正常，忽略即可
        return;
    }

    // 3. 处理业务数据 (42...)
    if (message.startsWith("42")) {
        QString jsonStr = message.mid(2);
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (!doc.isArray()) return;

        QJsonArray rootArr = doc.array();
        if (rootArr.isEmpty()) return;

        QString eventName = rootArr[0].toString();
        if (rootArr.size() < 2) return;
        QJsonValue dataVal = rootArr[1];

        if (eventName == "droneStatus") {
            if (dataVal.isArray()) parseDroneStatus(dataVal.toArray());
        }
        else if (eventName == "imageStatus") {
            if (dataVal.isArray()) parseImageStatus(dataVal.toArray());
        }
        else if (eventName == "info") {
            if (dataVal.isObject()) parseDeviceInfo(dataVal.toObject());
        }
    }
}

// 【新增】解析握手信息并启动心跳
void DetectionDriver::handleHandshake(const QString &payload)
{
    QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8());
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        if (obj.contains("pingInterval")) {
            int interval = obj["pingInterval"].toInt();
            // 建议比服务器要求的时间稍微短一点，比如减去 5秒，确保不超时
            // 如果服务器说 25000，我们 20000 发一次
            int timerInterval = (interval > 5000) ? (interval - 5000) : interval;

            m_heartbeatTimer->setInterval(timerInterval);
            m_heartbeatTimer->start();

            emit sigLog(QString("[侦测] 握手成功，心跳间隔: %1ms").arg(timerInterval));
        }
    }
}

// ... (以下 parseDroneStatus, parseImageStatus, parseDeviceInfo 保持不变) ...

void DetectionDriver::parseDroneStatus(const QJsonArray &dataArr)
{
    QList<DroneInfo> droneList;
    for (const auto &item : dataArr) {
        QJsonObject obj = item.toObject();
        if (!obj.contains("uav_info")) continue;
        QJsonObject info = obj["uav_info"].toObject();

        DroneInfo drone;
        drone.uav_id = info["uav_id"].toString();
        drone.model_name = info["model_name"].toString();
        drone.freq = info["freq"].toDouble();
        drone.whiteList = info["whiteList"].toBool();

        QString latStr = info["uav_lat"].toString();
        QString lngStr = info["uav_lng"].toString();
        drone.uav_lat = latStr.toDouble();
        drone.uav_lng = lngStr.toDouble();

        QString distStr = info["distance"].toString();
        if (distStr == "--") drone.distance = 0.0;
        else drone.distance = distStr.toDouble();

        droneList.append(drone);
    }
    emit sigDroneListUpdated(droneList);
}

void DetectionDriver::parseImageStatus(const QJsonArray &dataArr)
{
    QList<ImageInfo> imageList;
    for (const auto &item : dataArr) {
        QJsonObject obj = item.toObject();
        ImageInfo img;
        img.id = obj["id"].toString();
        img.freq = obj["freq"].toDouble();
        img.amplitude = obj["amplitude"].toDouble();
        img.type = obj["type"].toInt();
        if (img.id.endsWith("_fpv")) img.type = 1;
        imageList.append(img);
    }
    emit sigImageListUpdated(imageList);
}

void DetectionDriver::parseDeviceInfo(const QJsonObject &dataObj)
{
    double lat = dataObj["lat"].toDouble();
    double lng = dataObj["lng"].toDouble();
    emit sigDevicePositionUpdated(lat, lng);
}
