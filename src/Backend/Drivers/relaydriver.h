#ifndef RELAYDRIVER_H
#define RELAYDRIVER_H

#include <QObject>
#include <QMap>
#include "../HAL/tcpclient.h"

class RelayDriver : public QObject
{
    Q_OBJECT
public:
    explicit RelayDriver(QObject *parent = nullptr);

    // 连接设备
    void connectToDevice(const QString &ip, int port);

    // 单路控制 (channel: 1-7, on: true/false)
    void setChannel(int channel, bool on);

    // 全开/全关 (自动模式用)
    void setAll(bool on);

private:
    TcpClient *m_tcpClient;

    // 辅助：发送 Hex 数据
    void sendHex(const QByteArray &hex);

    // 协议表
    QByteArray getChannelCmd(int channel, bool on);
    QByteArray getAllCmd(bool on);
};

#endif // RELAYDRIVER_H
