#include "socketioclient.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

SocketIoClient::SocketIoClient(QObject *parent) : QObject(parent)
{
    m_webSocket = new QWebSocket();
    m_isManualClose = false;

    // 初始化重连定时器
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(3000);
    connect(m_reconnectTimer, &QTimer::timeout, this, &SocketIoClient::onReconnectTimerOut);

    // 连接成功
    connect(m_webSocket, &QWebSocket::connected, this, [this](){
        m_reconnectTimer->stop();
        qDebug() << "[SocketIO] 物理连接建立";
    });

    // 连接断开 -> 启动重连
    connect(m_webSocket, &QWebSocket::disconnected, this, [this](){
        emit disconnected();
        if (!m_isManualClose) {
            qDebug() << "[SocketIO] 连接断开，3秒后尝试重连...";
            m_reconnectTimer->start();
        }
    });

    // 发生错误 -> 启动重连
    connect(m_webSocket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError error){
        qDebug() << "[SocketIO Error]" << error;
        // 如果当前没连上，且定时器没跑，就启动它
        if (m_webSocket->state() != QAbstractSocket::ConnectedState && !m_isManualClose) {
            if (!m_reconnectTimer->isActive()) {
                m_reconnectTimer->start();
            }
        }
    });

    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &SocketIoClient::onTextMessageReceived);
}

void SocketIoClient::connectToServer(const QString &url)
{
    m_targetUrl = url;

    if (!m_targetUrl.endsWith("/")) m_targetUrl += "/";
    if (!m_targetUrl.contains("socket.io")) m_targetUrl += "socket.io/?EIO=4&transport=websocket";

    m_isManualClose = false;
    doConnect();
}

void SocketIoClient::doConnect()
{
    // 如果已经连接或正在连接，就不操作
    if (m_webSocket->state() == QAbstractSocket::ConnectedState ||
        m_webSocket->state() == QAbstractSocket::ConnectingState) {
        return;
    }

    qDebug() << "[SocketIO] 尝试连接..." << m_targetUrl;
    m_webSocket->open(QUrl(m_targetUrl));
}

void SocketIoClient::onReconnectTimerOut()
{
    doConnect();
}

void SocketIoClient::onTextMessageReceived(const QString &message)
{
    // --- 协议解析核心 ---
    // Socket.IO 协议代码：
    // '0' => Open (握手包)
    // '40' => Connect (连接成功)
    // '42' => Event (收到数据)

    // qDebug() << "[DEBUG] 收到原始包:" << message; // 调试用

    // 心跳保持 ===
    // 协议定义: '2' 是服务器发的 Ping
    if (message == "2") {
        // 我们必须回一个 '3' (Pong)
        m_webSocket->sendTextMessage("3");
        // qDebug() << "[SocketIO] 心跳回应 (Pong)"; // 嫌刷屏可以注释掉
        return;
    }

    if (message.startsWith("0")) {
        // 收到服务端握手包(0)后，必须回复连接指令(40)
        // 否则服务器不知道你要连哪个命名空间（默认是 /）
        m_webSocket->sendTextMessage("40");
        qDebug() << "[SocketIO] 发送握手响应 (40)";
    }
    else if (message.startsWith("40")) {
        // 收到 40 代表真正连上了
        qDebug() << "[SocketIO] 握手成功 (Connected)";
        emit connected();
    }
    else if (message.startsWith("42")) {
        // 处理事件数据: 42["event", data]
        QString jsonPart = message.mid(2);
        parseSocketIoMessage(jsonPart);
    }
}

void SocketIoClient::parseSocketIoMessage(const QString &msg)
{
    QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8());
    if (doc.isArray()) {
        QJsonArray arr = doc.array();
        if (arr.size() >= 2) {
            QString eventName = arr[0].toString();
            QJsonValue data = arr[1];

            // qDebug() << "[SocketIO Event]" << eventName;
            emit eventReceived(eventName, data);
        }
    }
}
