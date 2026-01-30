#ifndef RELAYDRIVER_H
#define RELAYDRIVER_H

#include <QObject>
#include <QTcpSocket>

class RelayDriver : public QObject
{
    Q_OBJECT
public:
    explicit RelayDriver(QObject *parent = nullptr);
    ~RelayDriver();

    // 连接继电器
    void connectToDevice(const QString &ip, int port);
    void disconnectDevice();

    // 控制接口
    void setAll(bool on);               // 全开/全关
    void setChannel(int channel, bool on); // 单通道控制

signals:
    // 【新增】日志信号
    void sigLog(const QString &msg);
    // 连接状态信号 (可选)
    void sigConnected(bool isConnected);

private slots:
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);
    void onReadyRead();

private:
    void sendCommand(const QByteArray &data);

    QTcpSocket *m_socket;
    QString m_targetIp;
    int m_targetPort;
};

#endif // RELAYDRIVER_H
