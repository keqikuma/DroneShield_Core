#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QDebug>
#include <QTimer>

#include "DataStructs.h" // 引入新结构体
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

    // ... (手动/自动模式切换函数保持不变，此处省略以节省篇幅) ...
    void setSystemMode(SystemMode mode);
    void stopAllBusiness();

    // 手动控制函数
    void setManualSpoofSwitch(bool enable);
    void setManualCircular();
    void setManualDirection(SpoofDirection dir);
    void setJammerConfig(const QList<JammerConfigData> &configs);
    void setManualJammer(bool enable);
    void setRelayChannel(int channel, bool on);
    void setRelayAll(bool on);

private slots:
    // --- 接收驱动层信号的槽 ---
    void onDroneListUpdated(const QList<DroneInfo> &drones);
    void onImageListUpdated(const QList<ImageInfo> &images);
    void onAlertCountUpdated(int count);
    void onDevicePositionUpdated(double lat, double lng);

private:
    void processDecision(bool hasDrone, double minDistance);
    void log(const QString &msg);

    SpoofDriver *m_spoofDriver;
    DetectionDriver *m_detectionDriver;
    JammerDriver *m_jammerDriver;
    RelayDriver *m_relayDriver;

    SystemMode m_currentMode;
    QTimer *m_monitorTimer;

    bool m_isAutoSpoofingRunning;
    bool m_isLinuxJammerRunning;
    bool m_isRelaySuppressionRunning;

signals:
    // --- 发送给 UI 的信号 ---
    void sigLogMessage(const QString &msg);

    // 全量无人机数据
    void sigDroneList(const QList<DroneInfo> &drones);
    // 全量图传数据
    void sigImageList(const QList<ImageInfo> &images);
    // 告警数量
    void sigAlertCount(int count);
    // 设备自身位置
    void sigSelfPosition(double lat, double lng);

    // 兼容旧逻辑的信号
    void sigTargetsUpdated(const QList<DroneInfo> &drones);
};

#endif // DEVICEMANAGER_H
