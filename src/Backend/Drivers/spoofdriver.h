/**
 *  spoofdriver.h
 *      Created on: 2026年1月21日
 *          Author: rustinchen
 */

#ifndef SPOOFDRIVER_H
#define SPOOFDRIVER_H

#include <QObject>
#include <QMap>
#include "../HAL/udpsender.h"

class SpoofDriver : public QObject
{
    Q_OBJECT
public:
    explicit SpoofDriver(QObject *parent = nullptr);

    // ==========================================
    // 核心业务接口
    // ==========================================

    /**
     * @brief 启动诱骗 (综合流程)
     * 会依次发送: 登录 -> 位置 -> 默认参数 -> 开启
     */
    void startSpoofing(double lat, double lon, double alt = 100.0);

    /**
     * @brief 停止诱骗
     * 发送 602 关闭指令
     */
    void stopSpoofing();

    // ==========================================
    // 细分指令集 (对应 PDF 文档)
    // ==========================================

    // [PDF 3.15] 登录/更新接收端信息 (619)
    void sendLogin();

    // [PDF 3.4] 设置功率衰减 (603)
    // type: 1=GPS_L1CA, 2=BDS_B1I ...
    void setAttenuation(int type, float value);

    // [PDF 3.5] 设置通道时延 (604)
    void setDelay(int type, float ns);

    // [PDF 3.9] 设置直线运动/初速度 (608)
    void setLinearMotion(float speed, float angle);

    // [PDF 3.11] 设置圆周运动 (610)
    void setCircularMotion(float radius, float cycle, int direction = 0);

    // [PDF 3.13] 设置系统时间 (613)
    void setSystemTime();

signals:
    // 上报设备状态 (解析 600 报文后发出)
    // isLocked: 晶振是否锁定 (iOcxoSta == 3)
    // isReady: 系统是否就绪 (iSysSta >= 3)
    void deviceStatusUpdated(bool isLocked, bool isReady);

private:
    UdpSender *m_udpSender;

    // 内部辅助：通道名称映射 (1 -> "GPS_L1CA")
    // 参考 udp.js 中的 channelName 对象
    QString getChannelName(int type);

    // 通用 UDP 发送辅助
    void sendUdpCmd(const QString &code, const QJsonObject &json);
};

#endif // SPOOFDRIVER_H
