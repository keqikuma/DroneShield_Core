#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QDebug>
#include <QTimer>

#include "Drivers/spoofdriver.h"
#include "Drivers/detectiondriver.h"
#include "Drivers/jammerdriver.h"
#include "Drivers/relaydriver.h"

// 系统模式
enum class SystemMode {
    Manual,
    Auto
};

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager();

    // ==========================================
    // 模式切换
    // ==========================================
    void setSystemMode(SystemMode mode);
    void stopAllBusiness();

    // ==========================================
    // 手动模式业务
    // ==========================================

    // 1. 诱骗控制
    void setManualSpoofSwitch(bool enable); // 手动总开关
    void setManualCircular();               // 手动圆周
    void setManualDirection(SpoofDirection dir); // 手动定向 (东/南/西/北)

    // 2. 干扰控制 (Linux)
    void setJammerConfig(const QList<JammerConfigData> &configs);
    void setManualJammer(bool enable);

    // 3. 压制控制 (Relay)
    void setRelayChannel(int channel, bool on);
    void setRelayAll(bool on);

private slots:
    // 侦测信号槽
    void onRealTimeDroneDetected(const QList<DroneInfo> &drones);
    void onRealTimeImageDetected(int count, const QString &desc);

private:
    // 核心自动决策
    void processDecision(bool hasDrone, double distance);
    void log(const QString &msg);

    // 驱动
    SpoofDriver *m_spoofDriver;
    DetectionDriver *m_detectionDriver;
    JammerDriver *m_jammerDriver;
    RelayDriver *m_relayDriver;

    // 状态
    SystemMode m_currentMode;
    QTimer *m_monitorTimer;

    // 运行标志位
    bool m_isAutoSpoofingRunning;
    bool m_isLinuxJammerRunning;
    bool m_isRelaySuppressionRunning;

signals:
    void sigLogMessage(const QString &msg);
    void sigTargetsUpdated(const QList<DroneInfo> &drones);
};

#endif // DEVICEMANAGER_H
