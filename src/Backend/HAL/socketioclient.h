#ifndef SOCKETIOCLIENT_H
#define SOCKETIOCLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QJsonObject>
#include <QTimer>

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
    void onReconnectTimerOut();

private:
    QWebSocket *m_webSocket;
    void parseSocketIoMessage(const QString &msg);

    // 重连机制变量
    QTimer *m_reconnectTimer;
    QString m_targetUrl;      // 保存目标地址
    bool m_isManualClose;     // 标记是否是用户手动关闭

    void doConnect();         // 内部连接执行函数
};

#endif // SOCKETIOCLIENT_H
