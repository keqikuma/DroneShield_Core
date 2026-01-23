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

SpoofDriver::SpoofDriver(const QString &hostIp, int hostPort, QObject *parent)
    : QObject(parent)
{
    m_udpSender = new UdpSender(this);

    // 设置 UDP 目标地址
    m_udpSender->setTarget(hostIp, hostPort);

    connect(m_udpSender, &UdpSender::statusDataReceived, this,
            [this](QJsonObject data){
                // ... (保持原有的状态解析逻辑) ...
                if (data.contains("iOcxoSta") && data.contains("iSysSta")) {
                    int ocxo = data["iOcxoSta"].toInt();
                    int sys  = data["iSysSta"].toInt();
                    bool isLocked = (ocxo == 3);
                    bool isReady  = (sys >= 3);
                    emit deviceStatusUpdated(isLocked, isReady);
                }
            });

    sendLogin();
}

// =================================================================
// 核心流程
// =================================================================

void SpoofDriver::startSpoofing(double lat, double lon, double alt)
{
    qDebug() << ">>> [SpoofDriver] 启动诱骗 (全协议版)";

    sendLogin();

    // 1. 设置位置 (601)
    QJsonObject pos;
    pos["sKey"] = Config::LURE_SKEY;
    pos["dbLon"] = lon;
    pos["dbLat"] = lat;
    pos["dbAlt"] = alt;
    sendUdpCmd("601", pos);

    // 2. 默认参数设置
    // 【修改】将 type 0 改为 1 (GPS_L1CA)，或者其他主要频段
    // 这样 getChannelName(1) 就能返回正确字符串，指令就能发出去
    setDelay(1, 1000 * 3.3);     // 设置 GPS L1 时延
    setAttenuation(1, 10.0);     // 设置 GPS L1 衰减

    // 如果需要设置其他频段，可以多写几行，或者写个循环
    // setAttenuation(2, 10.0); // BDS B1I
    // setAttenuation(3, 10.0); // GLO L1

    setLinearMotion(15.0, 0.0);

    // 3. 开启发射 (602)
    QJsonObject sw;
    sw["sKey"] = Config::LURE_SKEY;
    sw["iSwitch"] = 1;
    sendUdpCmd("602", sw);
}

void SpoofDriver::stopSpoofing()
{
    qDebug() << "<<< [SpoofDriver] 停止诱骗";

    QJsonObject sw;
    sw["sKey"] = Config::LURE_SKEY;
    sw["iSwitch"] = 0;
    sendUdpCmd("602", sw);
}

// =================================================================
// 完整指令集实现
// =================================================================

// 辅助：获取本机IP
QString SpoofDriver::getLocalIP()
{
    QString localIp = "127.0.0.1";
#ifdef SIMULATION_MODE
    return localIp;
#else
    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    for (const QHostAddress &address : QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost)
            localIp = address.toString();
    }
    return localIp;
#endif
}

void SpoofDriver::sendLogin()
{
    // 619 登录
    QJsonObject json;
    json["sKey"] = Config::LURE_SKEY;
    json["sIP"] = getLocalIP();
    json["iPort"] = 9098;
    sendUdpCmd("619", json);
}

void SpoofDriver::sendLogout()
{
    // 620 注销
    QJsonObject json;
    json["sKey"] = Config::LURE_SKEY;
    json["sIP"] = getLocalIP();
    json["iPort"] = 9098;
    sendUdpCmd("620", json);
}

void SpoofDriver::rebootDevice()
{
    // 605 重启设备
    QJsonObject json;
    json["sKey"] = Config::LURE_SKEY;
    json["sReboot"] = "REBOOT";
    sendUdpCmd("605", json);
}

void SpoofDriver::setHeartbeatCycle(int cycle)
{
    // 615 设置心跳周期
    QJsonObject json;
    json["sKey"] = Config::LURE_SKEY;
    json["iCycle"] = cycle; // 2-30秒
    sendUdpCmd("615", json);
}

void SpoofDriver::setSystemTime()
{
    // 613 同步时间
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

// --- 诱骗参数 ---

QString SpoofDriver::getChannelName(int type)
{
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
    // 603 功率衰减
    QString chName = getChannelName(type);
    if(chName == "Unknown") return;
    QJsonObject json;
    json["sKey"] = Config::LURE_SKEY;
    json["iType"] = type;
    json["fAtten" + chName] = value;
    sendUdpCmd("603", json);
}

void SpoofDriver::setDelay(int type, float ns)
{
    // 604 通道时延
    QString chName = getChannelName(type);
    if(chName == "Unknown") return;
    QJsonObject json;
    json["sKey"] = Config::LURE_SKEY;
    json["iType"] = type;
    json["fDelay" + chName] = ns;
    sendUdpCmd("604", json);
}

// --- 运动控制 ---

void SpoofDriver::setLinearMotion(float speed, float angle)
{
    // 608 初速度
    QJsonObject json;
    json["sKey"] = Config::LURE_SKEY;
    json["fInitSpeedVal"] = speed;
    json["fInitSpeedHead"] = angle;
    sendUdpCmd("608", json);
}

void SpoofDriver::setAcceleration(float acc, float angle)
{
    // 609 加速度
    QJsonObject json;
    json["sKey"] = Config::LURE_SKEY;
    json["fAccSpeedVal"] = acc;   // m/s^2
    json["fAccSpeedHead"] = angle; // 度
    sendUdpCmd("609", json);
}

void SpoofDriver::setCircularMotion(float radius, float cycle, int direction)
{
    // 610 圆周运动
    QJsonObject json;
    json["sKey"] = Config::LURE_SKEY;
    json["fCirRadius"] = radius;
    json["fCirCycle"] = cycle;
    json["iCirRotDir"] = direction;
    sendUdpCmd("610", json);
}

// --- 区域控制 ---

void SpoofDriver::setNoFlyZone(int id, double lat, double lon, int alt)
{
    // 705 设置禁飞区
    QJsonObject json;
    json["sKey"] = Config::LURE_SKEY;
    json["iNum"] = id;  // 0-9
    json["dbLon"] = lon;
    json["dbLat"] = lat;
    json["iAlt"] = alt; // PDF要求是 Int32
    sendUdpCmd("705", json);
}

void SpoofDriver::queryNoFlyZone()
{
    // 802 查询禁飞区
    QJsonObject json;
    json["sKey"] = Config::LURE_SKEY;
    json["sNoFly"] = "NoFlyZone";
    sendUdpCmd("802", json);
}

void SpoofDriver::sendUdpCmd(const QString &code, const QJsonObject &json)
{
    m_udpSender->sendCommand(code, json);
}
