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

    // ================= 业务接口 =================
    // 1. 基础诱骗 (定点)
    void startSpoofing(int type, double lat, double lon);

    // 2. 停止诱骗
    void stopSpoofing();

    // 3. 动态调整 (未实现 -> 现已补全)
    // 设置直线驱离 (角度 0-360)
    void setLinearSpoofing(double angle);

    // 设置圆周驱离 (半径, 周期)
    void setCircularSpoofing(double radius, double cycle);

private:
    TcpClient *m_rfClient;
    UdpSender *m_udpSender;

    // ================= TCP 硬件控制 (完全复刻 tcp.js) =================
    void configureRfBoard(int type); // 流程总管

    void sendFreq(int type);         // 0x01: 设置频率
    void sendTimeSlot(int slot);     // 0x05: 设置时隙
    void sendModule(int mode);       // 0x0C: 设置模组
    void sendAttenuation(int v1, int v2); // 0x02: 设置衰减
    void sendWaveFile(int type);     // 0x09: 下发文件

    // 通用协议封装 EB 90 ...
    void sendEb90Cmd(quint8 cmdCode, const QByteArray &payload);

    // ================= UDP 策略控制 (完全复刻 udp.js) =================
    void sendStrategy(double lat, double lon); // 601 + 602

    // 发送 UDP 包辅助
    void sendUdpCmd(const QString &code, const QJsonObject &json);
};

#endif // SPOOFDRIVER_H
