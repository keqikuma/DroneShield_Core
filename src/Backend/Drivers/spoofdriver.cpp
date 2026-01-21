/**
 *  spoofdriver.cpp
 *      Created on: 2026年1月21日
 *          Author: rustinchen
 */

#include "spoofdriver.h"
#include "../Consts.h"
#include <QDebug>
#include <QDateTime>
#include <QJsonObject>
#include <QHostAddress>
#include <QNetworkInterface>

SpoofDriver::SpoofDriver(QObject *parent) : QObject(parent)
{
    m_udpSender = new UdpSender(this);

    // 监听 UDP 回包 (特别是 600 状态包)
    // 注意：这里使用了上一轮建议你修改 udpsender.cpp 后应有的 statusDataReceived 信号
    // 如果还没改 udpsender.cpp，请务必把那个 emit statusDataReceived(obj) 加回去！
    connect(m_udpSender, &UdpSender::statusDataReceived, this,
            [this](QJsonObject data){
                // 解析 600 报文 (PDF 3.1)
                if (data.contains("iOcxoSta") && data.contains("iSysSta")) {
                    int ocxo = data["iOcxoSta"].toInt(); // 3=锁定
                    int sys  = data["iSysSta"].toInt();  // 3=就绪

                    bool isLocked = (ocxo == 3);
                    bool isReady  = (sys >= 3);

                    static bool lastLocked = false;
                    if (isLocked != lastLocked) {
                        qDebug() << "[SpoofDriver] 晶振锁定状态变更:" << isLocked;
                        lastLocked = isLocked;
                    }

                    emit deviceStatusUpdated(isLocked, isReady);
                }
            });

    // 初始化时尝试发送一次登录指令 (PDF 3.15)
    // 这对于某些设备是必须的，否则它不知道往哪里回传数据
    sendLogin();
}

// =================================================================
// 核心流程
// =================================================================

void SpoofDriver::startSpoofing(double lat, double lon, double alt)
{
    qDebug() << ">>> [SpoofDriver] 启动诱骗 (UDP Only)";

    // 1. 确保登录 (619)
    sendLogin();

    // 2. 设置位置 (601) [PDF 3.2]
    QJsonObject pos;
    pos["sKey"] = Config::LURE_SKEY;
    pos["dbLon"] = lon;
    pos["dbLat"] = lat;
    pos["dbAlt"] = alt;
    sendUdpCmd("601", pos);

    // 3. (可选) 设置一些默认参数，例如时延和功率
    // 根据 Node.js setmodele(3) 的逻辑，这里可以预设一些值
    // setDelay(1, 1000); // 示例

    // 4. 开启发射开关 (602) [PDF 3.3]
    QJsonObject sw;
    sw["sKey"] = Config::LURE_SKEY;
    sw["iSwitch"] = 1; // 1=开
    // iBkType 不传则默认全开
    sendUdpCmd("602", sw);
}

void SpoofDriver::stopSpoofing()
{
    qDebug() << "<<< [SpoofDriver] 停止诱骗";

    // 关闭发射开关 (602)
    QJsonObject sw;
    sw["sKey"] = Config::LURE_SKEY;
    sw["iSwitch"] = 0; // 0=关
    sendUdpCmd("602", sw);
}

// =================================================================
// 细分指令集实现
// =================================================================

void SpoofDriver::sendLogin()
{
    // [PDF 3.15] 命令 619
    QJsonObject json;
    json["sKey"] = Config::LURE_SKEY;

    // 获取本机 IP (简单实现，取第一个非本地 IP)
    QString localIp = "127.0.0.1";
    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    for (const QHostAddress &address : QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost)
            localIp = address.toString();
    }

#ifdef SIMULATION_MODE
    localIp = "127.0.0.1";
#endif

    json["sIP"] = localIp;
    json["iPort"] = 9098; // 控制端接收端口 (PDF 表1)

    sendUdpCmd("619", json);
}

QString SpoofDriver::getChannelName(int type)
{
    // 映射表参考 udp.js line 22-36
    static QMap<int, QString> map = {
        {1, "GPS_L1CA"}, {2, "BDS_B1I"}, {3, "GLO_L1"}, {4, "GAL_E1"},
        {5, "BDS_B2I"},  {6, "BDS_B3I"}, {7, "BDS_B1C"}, {8, "GPS_L2C"},
        {9, "GPS_L5"},   {10, "GLO_L2"}, {11, "GAL_E5A"}, {12, "GAL_E5B"},
        {13, "GAL_E6"}
    };
    return map.value(type, "Unknown");
}

void SpoofDriver::setAttenuation(int type, float value)
{
    // [PDF 3.4] 命令 603
    QString chName = getChannelName(type);
    if(chName == "Unknown") return;

    QJsonObject json;
    json["sKey"] = Config::LURE_SKEY;
    json["iType"] = type;
    // 动态 key: fAttenGPS_L1CA (参考 udp.js line 170)
    json["fAtten" + chName] = value;

    sendUdpCmd("603", json);
}

void SpoofDriver::setDelay(int type, float ns)
{
    // [PDF 3.5] 命令 604
    QString chName = getChannelName(type);
    if(chName == "Unknown") return;

    QJsonObject json;
    json["sKey"] = Config::LURE_SKEY;
    json["iType"] = type;
    // 动态 key: fDelayGPS_L1CA (参考 udp.js line 185)
    json["fDelay" + chName] = ns;

    sendUdpCmd("604", json);
}

void SpoofDriver::setLinearMotion(float speed, float angle)
{
    // [PDF 3.9] 命令 608
    QJsonObject json;
    json["sKey"] = Config::LURE_SKEY;
    json["fInitSpeedVal"] = speed;   // m/s
    json["fInitSpeedHead"] = angle;  // 度
    sendUdpCmd("608", json);
}

void SpoofDriver::setCircularMotion(float radius, float cycle, int direction)
{
    // [PDF 3.11] 命令 610
    QJsonObject json;
    json["sKey"] = Config::LURE_SKEY;
    json["fCirRadius"] = radius;     // 半径 m
    json["fCirCycle"] = cycle;       // 周期 s
    json["iCirRotDir"] = direction;  // 0顺 1逆
    sendUdpCmd("610", json);
}

void SpoofDriver::setSystemTime()
{
    // [PDF 3.13] 命令 613
    QDateTime now = QDateTime::currentDateTimeUtc();
    QJsonObject json;
    json["sKey"] = Config::LURE_SKEY;
    json["iYear"] = now.date().year();
    json["iMonth"] = now.date().month();
    json["iDay"] = now.date().day();
    json["iHour"] = now.time().hour();
    json["iMin"] = now.time().minute();
    json["iSec"] = now.time().second();
    sendUdpCmd("613", json);
}

void SpoofDriver::sendUdpCmd(const QString &code, const QJsonObject &json)
{
    m_udpSender->sendCommand(code, json);
}
