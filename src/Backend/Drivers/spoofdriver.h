#ifndef SPOOFDRIVER_H
#define SPOOFDRIVER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include "../HAL/udpsender.h"

// 定义驱离方向枚举
enum class SpoofDirection {
    North = 0,
    East = 90,
    South = 180,
    West = 270
};

class SpoofDriver : public QObject
{
    Q_OBJECT
public:
    explicit SpoofDriver(const QString &ip, int port, QObject *parent = nullptr);

    // ==========================================
    // 基础原子指令
    // ==========================================

    // 1. 设置经纬度 (CMD: 601)
    // 必须先设置经纬度，才能开启后续模式
    void setPosition(double lon, double lat, double alt);

    // 2. 射频总开关 (CMD: 602)
    // enable: true=开, false=关
    void setSwitch(bool enable);

    // 3. 开启圆周驱离 (CMD: 610)
    // radius: 半径(米), cycle: 周期(秒?)
    void startCircular(double radius = 100.0, double cycle = 50.0);

    // 4. 开启定向驱离 (CMD: 608)
    // speed: 速度(m/s), dir: 方向
    void startDirectional(SpoofDirection dir, double speed = 15.0);

    // 辅助：发送登录/心跳 (可选，视硬件需求)
    void sendLogin();

private:
    UdpSender *m_udpSender;

    // 鉴权密钥 (来自 udp.js)
    const QString SKEY = "a57502fcdc4e7412";

    // 辅助函数：构造协议包 FF + Length + Code + JSON
    void sendCommand(const QString &code, const QJsonObject &data);
    QString getLocalIP();
};

#endif // SPOOFDRIVER_H
