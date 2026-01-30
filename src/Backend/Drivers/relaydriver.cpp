#include "relaydriver.h"
#include <QDebug>

RelayDriver::RelayDriver(QObject *parent) : QObject(parent)
{
    m_socket = new QTcpSocket(this);

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
    Q_UNUSED(socketError);
    emit sigLog(QString("[压制] 连接错误: %1").arg(m_socket->errorString()));
}

void RelayDriver::onReadyRead()
{
    // 继电器回复的数据，暂时不需要处理，仅做调试
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

    // 打印发送的 Hex，方便对比协议表     // emit sigLog(QString("[压制TX] %1").arg(QString(data.toHex().toUpper())));
}

// ============================================================================
// 全开 / 全关 (根据协议表底部)
// ============================================================================
void RelayDriver::setAll(bool on)
{
    QByteArray cmd;
    if (on) {
        // 全开指令 (与你的网络助手日志完全一致)
        // FE 0F 00 00 00 20 04 FF FF FF FF F6 0B
        cmd = QByteArray::fromHex("FE0F0000002004FFFFFFFFF60B");
        emit sigLog("[指令] 压制全开 (All ON)");
    } else {
        // 全关指令 (与你的网络助手日志完全一致)
        // FE 0F 00 00 00 20 04 00 00 00 00 F7 9F
        cmd = QByteArray::fromHex("FE0F000000200400000000F79F");
        emit sigLog("[指令] 压制全关 (All OFF)");
    }

    // 【调试核心】打印 Qt 实际发送的 HEX 字符串
    // 请检查控制台输出，必须和网络助手里的一模一样，不能短！
    qDebug() << "------------------------------------------------";
    qDebug() << "[Relay Check] 计划发送:" << cmd.toHex().toUpper();
    qDebug() << "[Relay Check] 字节长度:" << cmd.size(); // 应该是 13
    qDebug() << "------------------------------------------------";

    if (!cmd.isEmpty()) {
        sendCommand(cmd);
    }
}

// ============================================================================
// 单路控制 (1-7路)
// ============================================================================
void RelayDriver::setChannel(int channel, bool on)
{
    QString hexStr;

    switch (channel) {
    case 1:
        hexStr = on ? "FE050000FF009835" : "FE0500000000D9C5";
        break;
    case 2:
        hexStr = on ? "FE050001FF00C9F5" : "FE05000100008805";
        break;
    case 3:
        hexStr = on ? "FE050002FF0039F5" : "FE05000200007805";
        break;
    case 4:
        hexStr = on ? "FE050003FF006835" : "FE050003000029C5";
        break;
    case 5:
        hexStr = on ? "FE050004FF00D9F4" : "FE05000400009804";
        break;
    case 6:
        hexStr = on ? "FE050005FF008834" : "FE0500050000C9C4";
        break;
    case 7:
        hexStr = on ? "FE050006FF007834" : "FE050006000039C4";
        break;
    default:
        emit sigLog(QString("[压制] 错误: 不支持的通道 %1 (仅支持 1-7)").arg(channel));
        return;
    }

    emit sigLog(QString("[指令] 压制通道 %1 -> %2").arg(channel).arg(on ? "ON" : "OFF"));
    sendCommand(QByteArray::fromHex(hexStr.toLatin1()));
}
