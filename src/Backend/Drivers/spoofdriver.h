/**
 *  spoofdriver.h
 *      Created on: 2026年1月21日
 *          Author: rustinchen
 */

#ifndef SPOOFDRIVER_H
#define SPOOFDRIVER_H

#include <QObject>
#include "../HAL/tcpclient.h"
#include "../HAL/udpsender.h"

class SpoofDriver : public QObject
{
    Q_OBJECT
public:
    explicit SpoofDriver(QObject *parent = nullptr);

    // 核心接口：开启诱骗
    // type: 0=GPS, 1=Glonass (参考 bk15bx)
    // lat, lon: 虚假坐标
    void startSpoofing(int type, double lat, double lon);

    void stopSpoofing();

private:
    TcpClient *m_rfClient;   // 控制板卡15
    UdpSender *m_udpSender;  // 控制逻辑单元

    // 内部步骤
    void configureRF(int type); // 对应 bk15bx
    void sendStrategy(double lat, double lon); // 对应 setdevpos + signal
};

#endif // SPOOFDRIVER_H
