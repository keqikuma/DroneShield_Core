/**
 *  spoofdriver.cpp
 *      Created on: 2026年1月21日
 *          Author: rustinchen
 */

#include "spoofdriver.h"
#include <QDebug>
#include <QHostAddress>
#include <QNetworkInterface>

SpoofDriver::SpoofDriver(const QString &ip, int port, QObject *parent)
    : QObject(parent)
{
    m_udpSender = new UdpSender(this);
    m_udpSender->setTarget(ip, port);

    // 启动时尝试登录 (可选)
    sendLogin();
}

// 核心协议封装 (FF + 4位长 + 3位码 + JSON)
void SpoofDriver::sendCommand(const QString &code, const QJsonObject &json)
{
    QJsonDocument doc(json);
    QByteArray jsonBytes = doc.toJson(QJsonDocument::Compact);

    // 计算长度 (4位，不足补0)
    int len = jsonBytes.length();
    QString lenStr = QString("%1").arg(len, 4, 10, QChar('0'));

    QByteArray packet;
    packet.append("FF");
    packet.append(lenStr.toUtf8());
    packet.append(code.toUtf8());
    packet.append(jsonBytes);

    m_udpSender->sendData(packet); // 假设 UdpSender 有 sendData(QByteArray) 接口
}

void SpoofDriver::setPosition(double lon, double lat, double alt)
{
    // CMD: 601
    QJsonObject json;
    json["sKey"] = SKEY;
    json["dbLon"] = lon;
    json["dbLat"] = lat;
    json["dbAlt"] = alt;
    sendCommand("601", json);
}

void SpoofDriver::setSwitch(bool enable)
{
    // CMD: 602
    QJsonObject json;
    json["sKey"] = SKEY;
    json["iSwitch"] = enable ? 1 : 0;
    sendCommand("602", json);
    qDebug() << "[Spoof] 射频开关 ->" << (enable ? "ON" : "OFF");
}

void SpoofDriver::startCircular(double radius, double cycle)
{
    // CMD: 610
    QJsonObject json;
    json["sKey"] = SKEY;
    json["fCirRadius"] = radius;
    json["fCirCycle"] = cycle;
    json["iCirRotDir"] = 0; // 0: 顺时针
    sendCommand("610", json);
    qDebug() << "[Spoof] 模式 -> 圆周驱离";
}

void SpoofDriver::startDirectional(SpoofDirection dir, double speed)
{
    // CMD: 608
    QJsonObject json;
    json["sKey"] = SKEY;
    json["fInitSpeedVal"] = speed;
    json["fInitSpeedHead"] = static_cast<int>(dir); // 0, 90, 180, 270
    sendCommand("608", json);
    qDebug() << "[Spoof] 模式 -> 定向驱离 (角度:" << static_cast<int>(dir) << ")";
}

void SpoofDriver::sendLogin()
{
    // 619 登录 (可选)
    QJsonObject json;
    json["sKey"] = SKEY;
    json["sIP"] = "127.0.0.1";
    json["iPort"] = 9098;
    sendCommand("619", json);
}

// 辅助函数
QString SpoofDriver::getLocalIP() { return "127.0.0.1"; }
