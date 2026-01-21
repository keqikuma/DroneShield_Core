#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>

class TcpClient : public QObject
{
    Q_OBJECT
public:
    explicit TcpClient(QObject *parent = nullptr);

    // 连接到服务器
    void connectToServer(const QString &ip, int port);

    // 发送指令 (自动处理断线重连)
    void sendCommand(const QByteArray &data);

    // 发送文件 (特殊协议：先发大小，再发内容)
    void sendFile(const QString &filePath);

    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void dataReceived(QByteArray data);

private slots:
    void onReadyRead();
    void onStateChanged(QAbstractSocket::SocketState state);

private:
    QTcpSocket *m_socket;
    QString m_targetIp;
    int m_targetPort;
};

#endif // TCPCLIENT_H
