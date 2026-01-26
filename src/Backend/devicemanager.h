#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QDebug>
#include <QTimer>

#include "Drivers/spoofdriver.h"
#include "Drivers/detectiondriver.h"
#include "Drivers/jammerdriver.h" // 包含干扰驱动头文件
#include "Drivers/relaydriver.h"

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
    void setSystemMode(SystemMode mode);

    // 1. 手动诱骗
    void setManualCircular();
    void setManualDirection(SpoofDirection dir);

    // 2. 手动干扰 (Linux板卡)
    // 【新增】配置参数接口
    void setJammerConfig(const QList<JammerConfigData> &configs);
    // 【新增】开关控制接口
    void setManualJammer(bool enable);

    // 手动继电器控制接口
    void setRelayChannel(int channel, bool on);
    void setRelayAll(bool on);

    // ==========================================
    // 总控
    // ==========================================
    // 【新增】停止所有正在运行的业务 (用于切换模式或目标消失时)
    void stopAllBusiness();

private slots:
    // 响应来自 DetectionDriver 的实时信号
    void onRealTimeDroneDetected(const QList<DroneInfo> &drones);
    void onRealTimeImageDetected(int count, const QString &desc);

private:
    // 内部核心决策逻辑
    void processDecision(bool hasDrone, double distance);

    // 内部辅助日志
    void log(const QString &msg);

private:
    // --- 驱动实例 ---
    SpoofDriver *m_spoofDriver;
    DetectionDriver *m_detectionDriver;
    JammerDriver *m_jammerDriver;
    RelayDriver *m_relayDriver;

    // --- 系统状态 ---
    SystemMode m_currentMode; // 当前系统模式
    QTimer *m_monitorTimer;   // 监控定时器 (断线重连)

    // --- 业务运行状态锁 ---
    bool m_isAutoSpoofingRunning;     // 自动诱骗是否运行中

    // 【新增】手动 Linux 干扰是否运行中
    bool m_isLinuxJammerRunning;

    // 【新增】自动 继电器压制是否运行中
    bool m_isRelaySuppressionRunning;

signals:
    // 日志信号
    void sigLogMessage(const QString &msg);
    // 目标列表更新信号
    void sigTargetsUpdated(const QList<DroneInfo> &drones);
    // 状态更新信号
    void sigStatusChanged(bool isAuto, bool isJamming, bool isSpoofing);
};

#endif // DEVICEMANAGER_H
