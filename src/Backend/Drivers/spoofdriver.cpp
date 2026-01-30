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
        // 绑定失败只打印日志，不让程序崩溃
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

// 接收数据处理 (解析基站坐标核心逻辑)
void SpoofDriver::onReadyRead()
{
    while (m_udpReceiver->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpReceiver->pendingDatagramSize());
        m_udpReceiver->readDatagram(datagram.data(), datagram.size());

        // 1. 转为字符串处理
        // 格式示例: FF0262599{"iSysSta":3, ... "dbFixLon":119.xxx ...}
        QString rawData = QString::fromUtf8(datagram);

        // 2. 简单的协议校验 (FF开头且长度足够)
        if (rawData.startsWith("FF") && rawData.length() > 10) {

            // 提取 JSON 部分 (第9位开始到最后)
            // 0-1: FF, 2-5: Length, 6-8: Code(599), 9...: JSON
            QString jsonStr = rawData.mid(9);

            // 3. 解析 JSON
            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
            if (doc.isObject()) {
                QJsonObject obj = doc.object();

                // 【核心】提取经纬度 (兼容 599 或 600 协议)
                if (obj.contains("dbFixLat") && obj.contains("dbFixLon")) {
                    double lat = obj["dbFixLat"].toDouble();
                    double lng = obj["dbFixLon"].toDouble();

                    // 只有当坐标有效(非0)时才更新
                    // 过滤掉经纬度为0或者极其微小的情况
                    if (lat > 1.0 && lng > 1.0) {
                        emit sigDevicePosition(lat, lng);

                        // 调试时可以打开下面这行，看是否收到坐标
                        // qDebug() << "[Spoof GPS] 基站坐标更新:" << lat << lng;
                    }
                }
            }
        }
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
        emit sigSpoofLog(QString("[诱骗异常] 发送失败: %1").arg(m_udpSender->errorString()));
    } else {
        // 正常发送不刷屏日志，仅调试输出
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
    json["sIP"] = getLocalIP();
    json["iPort"] = 9098; // 告诉硬件往 9098 发数据

    sendCommand("619", json);
    emit sigSpoofLog(QString("[系统] 发送登录包 -> LocalIP: %1, ListenPort: 9098").arg(json["sIP"].toString()));
}

// 智能获取本机 IP
QString SpoofDriver::getLocalIP()
{
    const QList<QHostAddress> list = QNetworkInterface::allAddresses();
    for (const QHostAddress &addr : list) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol && !addr.isLoopback()) {
            QString ip = addr.toString();
            // 关键：只匹配与诱骗设备同网段的 IP (192.168.10.x)
            if (ip.startsWith("192.168.10.")) {
                return ip;
            }
        }
    }
    // 没找到，返回默认值
    return "192.168.10.100";
}
