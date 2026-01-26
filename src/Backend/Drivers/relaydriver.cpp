#include "relaydriver.h"
#include <QDebug>

RelayDriver::RelayDriver(QObject *parent) : QObject(parent)
{
    m_tcpClient = new TcpClient(this);
}

void RelayDriver::connectToDevice(const QString &ip, int port)
{
    m_tcpClient->connectToServer(ip, port);
}

void RelayDriver::sendHex(const QByteArray &hex)
{
    if (m_tcpClient->isConnected()) {
        m_tcpClient->sendCommand(hex);
    } else {
        qWarning() << "[Relay] 设备未连接，无法发送:" << hex.toHex();
    }
}

void RelayDriver::setChannel(int channel, bool on)
{
    if (channel < 1 || channel > 7) return;
    qDebug() << "[Relay] 单路控制 -> 通道:" << channel << " 状态:" << (on ? "ON" : "OFF");
    sendHex(getChannelCmd(channel, on));
}

void RelayDriver::setAll(bool on)
{
    qDebug() << "[Relay] 总控 -> " << (on ? "全开 (ALL ON)" : "全关 (ALL OFF)");
    sendHex(getAllCmd(on));
}

// === 协议字典 (硬编码最为稳妥) ===
QByteArray RelayDriver::getChannelCmd(int channel, bool on)
{
    // 格式：FE 05 00 [CH-1] [FF/00] 00 [CRC_L] [CRC_H]
    // 既然只有7路，且 CRC 校验麻烦，直接查表法
    if (on) {
        switch(channel) {
        case 1: return QByteArray::fromHex("FE050000FF009835");
        case 2: return QByteArray::fromHex("FE050001FF00C9F5");
        case 3: return QByteArray::fromHex("FE050002FF0039F5");
        case 4: return QByteArray::fromHex("FE050003FF006835");
        case 5: return QByteArray::fromHex("FE050004FF00D9F4");
        case 6: return QByteArray::fromHex("FE050005FF008834");
        case 7: return QByteArray::fromHex("FE050006FF007834");
        }
    } else {
        switch(channel) {
        case 1: return QByteArray::fromHex("FE0500000000D9C5");
        case 2: return QByteArray::fromHex("FE05000100008805");
        case 3: return QByteArray::fromHex("FE05000200007805");
        case 4: return QByteArray::fromHex("FE050003000029C5");
        case 5: return QByteArray::fromHex("FE05000400009804");
        case 6: return QByteArray::fromHex("FE0500050000C9C4");
        case 7: return QByteArray::fromHex("FE050006000039C4");
        }
    }
    return QByteArray();
}

QByteArray RelayDriver::getAllCmd(bool on)
{
    if (on) {
        return QByteArray::fromHex("FE0F0000002004FFFFFFFFF60B");
    } else {
        // 根据你的文本描述
        return QByteArray::fromHex("FE0F000000200400000000F79F");
    }
}
