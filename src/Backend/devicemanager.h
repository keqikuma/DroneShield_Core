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
    void stopAllBusiness(); // 立即停止所有业务

    // 手动控制函数
    void setManualSpoofSwitch(bool enable);
    void setManualCircular();
    void setManualDirection(SpoofDirection dir);
    void setJammerConfig(const QList<JammerConfigData> &configs);
    void setManualJammer(bool enable);
    void setRelayChannel(int channel, bool on);
    void setRelayAll(bool on);

private slots:
    void onDroneListUpdated(const QList<DroneInfo> &drones);
    void onImageListUpdated(const QList<ImageInfo> &images);
    void onAlertCountUpdated(int count);
    void onDevicePositionUpdated(double lat, double lng);

    // 【新增】防抖定时器超时槽函数
    void onStopDefenseTimeout();

private:
    void processDecision(bool hasDrone, double minDistance);
    void log(const QString &msg);

    SpoofDriver *m_spoofDriver;
    DetectionDriver *m_detectionDriver;
    JammerDriver *m_jammerDriver;
    RelayDriver *m_relayDriver;

    SystemMode m_currentMode;

    // 【新增】防抖定时器
    QTimer *m_stopDefenseTimer;

    bool m_isAutoSpoofingRunning;
    bool m_isLinuxJammerRunning;
    bool m_isRelaySuppressionRunning;

signals:
    void sigLogMessage(const QString &msg);
    void sigDroneList(const QList<DroneInfo> &drones);
    void sigImageList(const QList<ImageInfo> &images);
    void sigAlertCount(int count);
    void sigSelfPosition(double lat, double lng);
    void sigTargetsUpdated(const QList<DroneInfo> &drones);
};

#endif // DEVICEMANAGER_H
