#ifndef SOCKETIOCLIENT_H
#define SOCKETIOCLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QJsonObject>

class SocketIoClient : public QObject
{
    Q_OBJECT
public:
    explicit SocketIoClient(QObject *parent = nullptr);
    void connectToServer(const QString &url);

signals:
    void connected();
    void disconnected();
    // 收到事件信号：eventName (如 "droneStatus"), data (JSON数据)
    void eventReceived(const QString &eventName, const QJsonValue &data);

private slots:
    void onTextMessageReceived(const QString &message);

private:
    QWebSocket *m_webSocket;
    void parseSocketIoMessage(const QString &msg);
};

#endif // SOCKETIOCLIENT_H
