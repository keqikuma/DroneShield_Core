#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QDebug>
#include <QTimer>

#include "DataStructs.h"
#include "Drivers/spoofdriver.h"
#include "Drivers/detectiondriver.h"
#include "Drivers/jammerdriver.h"
#include "Drivers/relaydriver.h"

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

    void setSystemMode(SystemMode mode);
    void stopAllBusiness();

    // 手动控制
    void setManualSpoofSwitch(bool enable);
    void setManualCircular();
    void setManualDirection(SpoofDirection dir);
    void setJammerConfig(const QList<JammerConfigData> &configs);
    void setManualJammer(bool enable);
    void setRelayChannel(int channel, bool on);
    void setRelayAll(bool on);

private slots:
    // 数据接收槽
    void onDroneListUpdated(const QList<DroneInfo> &drones);
    void onImageListUpdated(const QList<ImageInfo> &images); // 【新增】图传也能触发
    void onAlertCountUpdated(int count);
    void onDevicePositionUpdated(double lat, double lng);
    void onStopDefenseTimeout();

private:
    // 核心决策函数
    void processDecision(bool hasThreat, double minDistance);
    void log(const QString &msg);

    SpoofDriver *m_spoofDriver;
    DetectionDriver *m_detectionDriver;
    JammerDriver *m_jammerDriver;
    RelayDriver *m_relayDriver;

    SystemMode m_currentMode;
    QTimer *m_stopDefenseTimer;

    // 状态标志位
    bool m_isAutoSpoofingRunning;
    bool m_isRelaySuppressionRunning;

    // 辅助：记录上一次是否有图传威胁（用于合并判断）
    bool m_hasImageThreat;

signals:
    void sigLogMessage(const QString &msg);
    void sigDroneList(const QList<DroneInfo> &drones);
    void sigImageList(const QList<ImageInfo> &images);
    void sigAlertCount(int count);
    void sigSelfPosition(double lat, double lng);
    void sigTargetsUpdated(const QList<DroneInfo> &drones);
};

#endif // DEVICEMANAGER_H
