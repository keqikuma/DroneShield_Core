#include "socketioclient.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

SocketIoClient::SocketIoClient(QObject *parent) : QObject(parent)
{
    m_webSocket = new QWebSocket();
    connect(m_webSocket, &QWebSocket::connected, this, &SocketIoClient::connected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &SocketIoClient::disconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &SocketIoClient::onTextMessageReceived);

    connect(m_webSocket, &QWebSocket::errorOccurred, this, [](QAbstractSocket::SocketError error){
        qDebug() << "[SocketIO Error]" << error;
    });
}

void SocketIoClient::connectToServer(const QString &url)
{
    // Socket.IO 通常连接路径是 /socket.io/?EIO=4&transport=websocket
    QString fullUrl = url;
    if (!fullUrl.endsWith("/")) fullUrl += "/";
    fullUrl += "socket.io/?EIO=4&transport=websocket";

    qDebug() << "[SocketIO] 连接中..." << fullUrl;
    m_webSocket->open(QUrl(fullUrl));
}

void SocketIoClient::onTextMessageReceived(const QString &message)
{
    // --- 协议解析核心 ---
    // Socket.IO 协议代码：
    // '0' => Open (握手包)
    // '40' => Connect (连接成功)
    // '42' => Event (收到数据)

    // qDebug() << "[DEBUG] 收到原始包:" << message; // 调试用

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
