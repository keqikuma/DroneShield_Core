#include "relaydriver.h"
#include <QDebug>

RelayDriver::RelayDriver(QObject *parent) : QObject(parent)
{
    m_socket = new QTcpSocket(this);
    // 关闭 Nagle，确保小包（如 11 字节全开指令）立即以单包发出，避免分包导致设备只解析到前几个字节
    m_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    connect(m_socket, &QTcpSocket::connected, this, &RelayDriver::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &RelayDriver::onDisconnected);
    // Qt 5.15+ 使用 errorOccurred
    connect(m_socket, &QTcpSocket::errorOccurred, this, &RelayDriver::onErrorOccurred);
}

RelayDriver::~RelayDriver()
{
    if (m_socket->isOpen()) {
        m_socket->close();
    }
}

void RelayDriver::connectToDevice(const QString &ip, int port)
{
    m_targetIp = ip;
    m_targetPort = port;

    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        if (m_socket->peerAddress().toString() == ip && m_socket->peerPort() == port) {
            return;
        }
        m_socket->abort();
    }

    emit sigLog(QString("[压制] 正在连接 TCP -> %1:%2 ...").arg(ip).arg(port));
    m_socket->connectToHost(ip, port);
}

void RelayDriver::disconnectDevice()
{
    m_socket->close();
}

void RelayDriver::onConnected()
{
    emit sigLog("[压制] TCP 连接成功!");
    emit sigConnected(true);
}

void RelayDriver::onDisconnected()
{
    emit sigLog("[压制] TCP 连接断开");
    emit sigConnected(false);
}

void RelayDriver::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    emit sigLog(QString("[压制] 连接错误: %1").arg(m_socket->errorString()));

    // 【新增】将错误向上抛出，以便 DeviceManager 捕获并切换 IP
    emit sigError(socketError);
}

void RelayDriver::onReadyRead()
{
    // 继电器回复的数据，暂时不需要处理
    // QByteArray data = m_socket->readAll();
}

void RelayDriver::sendCommand(const QByteArray &data)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        // 尝试自动重连
        m_socket->connectToHost(m_targetIp, m_targetPort);
        emit sigLog("[压制] 发送失败: 未连接 (尝试重连...)");
        return;
    }

    m_socket->write(data);
    m_socket->flush();
    // 等待本帧数据真正写入 socket，避免协议被拆包或未发完就返回导致只开部分继电器
    if (!m_socket->waitForBytesWritten(1000)) {
        emit sigLog(QString("[压制] 写入超时或未完成，已写: %1 字节").arg(data.size()));
    }
}

// ============================================================================
// 全开 / 全关（工程师协议：11 字节）
// ============================================================================
void RelayDriver::setAll(bool on)
{
    QByteArray cmd;
    if (on) {
        // 全开: FE 0F 00 00 00 10 02 FF FF A6 64
        cmd = QByteArray::fromHex("FE0F0000001002FFFFA664");
        emit sigLog("[指令] 压制全开 (All ON)");
    } else {
        // 全关: FE 0F 00 00 00 10 02 00 00 A7 D4
        cmd = QByteArray::fromHex("FE0F00000010020000A7D4");
        emit sigLog("[指令] 压制全关 (All OFF)");
    }

    const int kAllCmdLen = 11;
    if (cmd.size() != kAllCmdLen) {
        emit sigLog(QString("[压制] 全开/全关指令长度错误: 期望 %1 字节，实际 %2").arg(kAllCmdLen).arg(cmd.size()));
        return;
    }

    qDebug() << "[Relay] 全" << (on ? "开" : "关") << "指令:" << cmd.toHex(' ').toUpper() << "长度:" << cmd.size();
    sendCommand(cmd);
}

// ============================================================================
// 单路控制：通道 1=433, 2=915, 3=1.2, 4=1.5, 5=2.4, 6=5.2, 7=5.8（工程师表格）
// ============================================================================
void RelayDriver::setChannel(int channel, bool on)
{
    QString hexStr;

    switch (channel) {
    case 1: hexStr = on ? "FE050007FF0029F4" : "FE05000700006804"; break; // 433
    case 2: hexStr = on ? "FE050001FF00C9F5" : "FE05000100008805"; break; // 915
    case 3: hexStr = on ? "FE050002FF0039F5" : "FE05000200007805"; break; // 1.2
    case 4: hexStr = on ? "FE050003FF006835" : "FE050003000029C5"; break; // 1.5
    case 5: hexStr = on ? "FE050004FF00D9F4" : "FE05000400009804"; break; // 2.4
    case 6: hexStr = on ? "FE050005FF008834" : "FE0500050000C9C4"; break; // 5.2
    case 7: hexStr = on ? "FE050006FF007834" : "FE050006000039C4"; break; // 5.8
    default:
        emit sigLog(QString("[压制] 错误: 不支持的通道 %1 (仅支持 1-7)").arg(channel));
        return;
    }

    emit sigLog(QString("[指令] 压制通道 %1 -> %2").arg(channel).arg(on ? "ON" : "OFF"));
    sendCommand(QByteArray::fromHex(hexStr.toLatin1()));
}
