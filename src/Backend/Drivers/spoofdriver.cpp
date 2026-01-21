/**
 *  spoofdriver.cpp
 *      Created on: 2026年1月21日
 *          Author: rustinchen
 */

#include "spoofdriver.h"
#include "../Consts.h"
#include <QThread>

SpoofDriver::SpoofDriver(QObject *parent) : QObject(parent)
{
    // 初始化通信对象
    m_rfClient = new TcpClient(this);
    m_rfClient->connectToServer(Config::IP_BOARD_15, Config::PORT_BOARD_15); // 连接射频板

    m_udpSender = new UdpSender(this);
}

void SpoofDriver::startSpoofing(int type, double lat, double lon)
{
    qDebug() << ">>> [Spoof] 启动诱骗流程...";

    // 步骤 1: 配置射频硬件 (TCP)
    // 对应 tcp.js 中的 bk15bx(1)
    configureRF(type);

    // 步骤 2: 下发诱骗策略 (UDP)
    // 对应 udp.js 中的 setdevpos 和 signal
    sendStrategy(lat, lon);
}

void SpoofDriver::configureRF(int type)
{
    // 模拟 bk15bx 的逻辑
    // 1. 设置中心频率 (GPS=1581, Glonass=1603)
    int freq = (type == 0) ? 1581 : 1603;

    // 这里调用 m_rfClient 的通用指令发送函数 (需在 TcpClient 实现 sendCmd)
    // 示例伪代码，实际需要按 EB 90 协议拼包
    // m_rfClient->sendHexCommand("EB 90 00 0A 01 ...");
    qDebug() << "[Spoof-TCP] 配置板卡15频率:" << freq;

    // 2. 发送波形文件 (模拟)
    // m_rfClient->sendFile("path/to/gps.dat");
}

void SpoofDriver::sendStrategy(double lat, double lon)
{
    // 对应 udp.js setdevpos (Encode 601)
    QJsonObject posObj;
    posObj["sKey"] = Config::LURE_SKEY;
    posObj["dbLat"] = lat;
    posObj["dbLon"] = lon;
    posObj["dbAlt"] = 100.0; // 高度固定
    m_udpSender->sendCommand("601", posObj);

    // 对应 udp.js signal(1) (Encode 602) -> 开关开启
    QJsonObject switchObj;
    switchObj["sKey"] = Config::LURE_SKEY;
    switchObj["iSwitch"] = 1;
    m_udpSender->sendCommand("602", switchObj);
}

void SpoofDriver::stopSpoofing()
{
    // 发送关闭指令
    QJsonObject switchObj;
    switchObj["sKey"] = Config::LURE_SKEY;
    switchObj["iSwitch"] = 0;
    m_udpSender->sendCommand("602", switchObj);
}
