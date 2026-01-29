#include "spoofdriver.h"
#include <QDebug>
#include <QNetworkInterface>
#include <QDateTime>

SpoofDriver::SpoofDriver(const QString &targetIp, int targetPort, QObject *parent)
    : QObject(parent)
{
    // 1. 初始化发送端
    m_targetAddr = QHostAddress(targetIp);
    m_targetPort = targetPort;
    m_udpSender = new QUdpSocket(this);

    // 2. 初始化接收端 (监听 9098)
    m_udpReceiver = new QUdpSocket(this);
    // ShareAddress 允许端口复用，防止被占用报错
    if (m_udpReceiver->bind(QHostAddress::AnyIPv4, 9098, QUdpSocket::ShareAddress)) {
        qDebug() << "[SpoofDriver] 成功绑定本地端口: 9098";
        connect(m_udpReceiver, &QUdpSocket::readyRead, this, &SpoofDriver::onReadyRead);
    } else {
        qCritical() << "[SpoofDriver] 绑定 9098 失败:" << m_udpReceiver->errorString();
    }

    // 3. 启动时自动发送登录包
    // 延时 100ms 发送，确保 Socket 准备就绪
    QMetaObject::invokeMethod(this, "sendLogin", Qt::QueuedConnection);
}

SpoofDriver::~SpoofDriver()
{
    if (m_udpSender) m_udpSender->close();
    if (m_udpReceiver) m_udpReceiver->close();
}

// 接收数据处理
void SpoofDriver::onReadyRead()
{
    while (m_udpReceiver->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpReceiver->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        m_udpReceiver->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        // 你仍然可以在 IDE 的输出窗口看调试信息，但不要发给 UI
        // qDebug() << "[Spoof RX] " << datagram.toHex();

        // -----------------------------------------------------------
        // ❌ 屏蔽下面这行，界面就不会显示接收到的协议了
        // -----------------------------------------------------------
        // QString logMsg = QString("[Spoof RX] Len=%1").arg(datagram.size());
        // emit sigSpoofLog(logMsg);
    }
}

// 核心协议封装
void SpoofDriver::sendCommand(const QString &code, const QJsonObject &json)
{
    QJsonDocument doc(json);
    QByteArray jsonBytes = doc.toJson(QJsonDocument::Compact);

    int len = jsonBytes.length();
    QString lenStr = QString("%1").arg(len, 4, 10, QChar('0'));

    QByteArray packet;
    packet.append("FF");
    packet.append(lenStr.toUtf8());
    packet.append(code.toUtf8());
    packet.append(jsonBytes);

    qint64 ret = m_udpSender->writeDatagram(packet, m_targetAddr, m_targetPort);


    if (ret == -1) {
        // ❌ 错误一定要显示
        emit sigSpoofLog(QString("[诱骗异常] 发送失败: %1").arg(m_udpSender->errorString()));
    } else {
        // ✅ 正常发送不要显示了，否则自动模式下也会一直刷屏
        // emit sigSpoofLog(QString("[Spoof TX] Cmd: %1").arg(code));

        // 如果你想在 IDE 里看，可以用 qDebug
        qDebug() << "[Spoof TX] " << code << jsonBytes;
    }
}

// ================= 业务指令 =================

void SpoofDriver::setPosition(double lon, double lat, double alt)
{
    // CMD: 601 设置虚假位置
    QJsonObject json;
    json["sKey"] = SKEY;
    json["dbLon"] = lon;
    json["dbLat"] = lat;
    json["dbAlt"] = alt;
    sendCommand("601", json);
}

void SpoofDriver::setSwitch(bool enable)
{
    // CMD: 602 射频开关
    QJsonObject json;
    json["sKey"] = SKEY;
    json["iSwitch"] = enable ? 1 : 0;
    sendCommand("602", json);
    emit sigSpoofLog(QString("[指令] 射频开关 -> %1").arg(enable ? "ON" : "OFF"));
}

void SpoofDriver::startCircular(double radius, double cycle)
{
    // CMD: 610 圆周驱离
    QJsonObject json;
    json["sKey"] = SKEY;
    json["fCirRadius"] = radius;
    json["fCirCycle"] = cycle;
    json["iCirRotDir"] = 0; // 0: 顺时针
    sendCommand("610", json);
    emit sigSpoofLog(QString("[指令] 模式 -> 圆周驱离 (R=%1)").arg(radius));
}

void SpoofDriver::startDirectional(SpoofDirection dir, double speed)
{
    // CMD: 608 定向驱离
    QJsonObject json;
    json["sKey"] = SKEY;
    json["fInitSpeedVal"] = speed;
    json["fInitSpeedHead"] = static_cast<int>(dir);
    sendCommand("608", json);
    emit sigSpoofLog(QString("[指令] 模式 -> 定向驱离 (角度:%1)").arg(static_cast<int>(dir)));
}

void SpoofDriver::sendLogin()
{
    // CMD: 619 登录
    QJsonObject json;
    json["sKey"] = SKEY;

    // 获取本机在 192.168.10.x 网段的 IP
    QString myIp = getLocalIP();

    json["sIP"] = myIp;
    json["iPort"] = 9098; // 告诉硬件往 9098 发数据

    sendCommand("619", json);

    emit sigSpoofLog(QString("[系统] 发送登录包 -> LocalIP: %1, ListenPort: 9098").arg(myIp));
}

// 智能获取本机 IP
QString SpoofDriver::getLocalIP()
{
    const QList<QHostAddress> list = QNetworkInterface::allAddresses();
    for (const QHostAddress &addr : list) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol && !addr.isLoopback()) {
            QString ip = addr.toString();
            // 关键：只匹配与诱骗设备同网段的 IP
            if (ip.startsWith("192.168.10.")) {
                return ip;
            }
        }
    }
    // 没找到，返回默认值提示用户检查网络
    return "192.168.10.100";
}
