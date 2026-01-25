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
    // 简易 Socket.IO 协议解析
    // 0: Open, 40: Connect, 42: Event

    if (message.startsWith("0")) {
        // 握手成功
    } else if (message.startsWith("40")) {
        emit connected();
    } else if (message.startsWith("42")) {
        // 42["event_name", data]
        QString jsonPart = message.mid(2); // 去掉 "42"
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
            emit eventReceived(eventName, data);
        }
    }
}
