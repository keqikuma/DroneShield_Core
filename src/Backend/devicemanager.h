#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QDebug>
#include "Drivers/spoofdriver.h"
// 引入侦测和干扰驱动
#include "Drivers/detectiondriver.h"
#include "Drivers/jammerdriver.h"

// 定义方向枚举
enum class SpoofDirection {
    North, // 北 (0度)
    East,  // 东 (90度)
    South, // 南 (180度)
    West   // 西 (270度)
};

// 定义工作模式
enum class SystemMode {
    Manual, // 手动
    Auto    // 自动
};

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager();

    // ==========================================
    // 基础控制
    // ==========================================
    void startSpoofing(double lat, double lon, double alt = 100.0);
    void stopSpoofing();

    // ==========================================
    // 手动模式业务接口
    // ==========================================
    void setManualCircular();
    void setManualDirection(SpoofDirection dir);
    void setSystemMode(SystemMode mode);

    // ==========================================
    // 自动模式核心接口
    // ==========================================
    // /**
    //  * @brief 模拟接收侦测数据 (在这个阶段我们手动调用它来模拟)
    //  * @param hasDrone 是否发现无人机
    //  * @param distance 目标距离 (米)
    //  */
    // void updateDetection(bool hasDrone, double distance);

private slots:
    // 响应来自 DetectionDriver 的实时信号
    void onRealTimeDroneDetected(const QList<DroneInfo> &drones);
    void onRealTimeImageDetected(int count, const QString &desc);

private:
    // 内部核心决策逻辑
    void processDecision(bool hasDrone, double distance);

private:
    SpoofDriver *m_spoofDriver;

    // 侦测与干扰驱动
    DetectionDriver *m_detectionDriver;
    JammerDriver *m_jammerDriver;

    SystemMode m_currentMode; // 当前系统模式

    // 状态锁
    bool m_isAutoSpoofingRunning; // 记录诱骗是否在运行
    bool m_isJammingRunning;      // 记录干扰是否在运行
};

#endif // DEVICEMANAGER_H
