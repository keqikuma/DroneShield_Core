#ifndef SPOOFDRIVER_H
#define SPOOFDRIVER_H

#include <QObject>
#include <QMap>
#include "../HAL/udpsender.h"

class SpoofDriver : public QObject
{
    Q_OBJECT
public:
    explicit SpoofDriver(const QString &hostIp, int hostPort, QObject *parent = nullptr);

    // ==========================================
    // 核心业务接口
    // ==========================================

    // 启动/停止
    void startSpoofing(double lat, double lon, double alt = 100.0);
    void stopSpoofing();

    // ==========================================
    // 完整指令集
    // ==========================================

    // --- 基础配置 ---
    void sendLogin();                           // 619 登录
    void sendLogout();                          // 620 注销/断开
    void rebootDevice();                        // 605 重启设备
    void setHeartbeatCycle(int cycle);          // 615 设置心跳周期
    void setSystemTime();                       // 613 同步时间

    // --- 诱骗参数 ---
    void setAttenuation(int type, float value); // 603 功率衰减
    void setDelay(int type, float ns);          // 604 通道时延

    // --- 运动控制 ---
    void setLinearMotion(float speed, float angle); // 608 初速度
    void setAcceleration(float acc, float angle);   // 609 加速度
    void setCircularMotion(float radius, float cycle, int direction = 0); // 610 圆周

    // --- 区域控制 ---
    // id: 0-9, alt: 高度(米)
    void setNoFlyZone(int id, double lat, double lon, int alt); // 705 设置禁飞区
    void queryNoFlyZone();                                      // 802 查询禁飞区

signals:
    // 上报设备状态 (解析 600 报文后发出)
    void deviceStatusUpdated(bool isLocked, bool isReady);

private:
    UdpSender *m_udpSender;

    // 内部辅助
    QString getChannelName(int type);
    void sendUdpCmd(const QString &code, const QJsonObject &json);
    QString getLocalIP(); // 辅助函数：获取本机IP
};

#endif // SPOOFDRIVER_H
