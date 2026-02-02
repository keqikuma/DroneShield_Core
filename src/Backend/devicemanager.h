#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QDebug>
#include <QTimer>
#include <QAbstractSocket> // 用于 SocketError
#include <QtMath>

#include "DataStructs.h"
#include "Drivers/spoofdriver.h"
#include "Drivers/detectiondriver.h"
#include "Drivers/jammerdriver.h"
#include "Drivers/relaydriver.h"
#include "../Utils/configloader.h" // 【新增】引入配置加载器
#include "coordinatemanager.h"

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

    // 手动控制接口
    void setManualSpoofSwitch(bool enable);
    void setManualCircular();
    void setManualDirection(SpoofDirection dir);
    void setJammerConfig(const QList<JammerConfigData> &configs);
    void setManualJammer(bool enable);
    void setRelayChannel(int channel, bool on);
    void setRelayAll(bool on);

private slots:
    // --- 数据接收槽 ---
    void onDroneListUpdated(const QList<DroneInfo> &drones);
    void onImageListUpdated(const QList<ImageInfo> &images);
    void onAlertCountUpdated(int count);
    void onDevicePositionUpdated(double lat, double lng);

    // --- 逻辑槽 ---
    void onStopDefenseTimeout();

    // --- 【新增】故障回退槽 ---
    void onRelayError(QAbstractSocket::SocketError error); // 继电器连接失败
    void onDetectDisconnected();                           // 侦测断开

    // 【新增】自动循环逻辑槽
    void onAutoCycleTimeout();     // 10秒到了，暂停一下
    void onObservationTimeout();   // 观察结束，决定是否继续

    // 【新增】辅助函数：控制功放开关
    void controlAmp(bool open);

    // 【新增】功放连接错误处理
    void onAmpError(QAbstractSocket::SocketError error);
    void onAmp2Error(QAbstractSocket::SocketError error);

private:
    // 核心决策函数
    void processDecision(bool hasThreat, double distance);
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

    // 【新增】记录手动诱骗状态，用于坐标锁定
    bool m_isManualSpoofing = false;

    // 辅助：记录上一次是否有图传威胁
    bool m_hasImageThreat;

    // 动态获取的基站经纬度
    double m_baseLat = 0.0;
    double m_baseLng = 0.0;

    // --- 【新增】配置与回退管理 ---
    NetConfig m_currDetectCfg;  // 当前侦测配置
    NetConfig m_currRelayCfg;   // 当前继电器配置
    bool m_detectFallbackUsed = false; // 是否已回退过
    bool m_relayFallbackUsed = false;  // 是否已回退过

    // 【新增】坐标管理器
    CoordinateManager *m_coordManager;

    // 【新增】自动模式循环定时器
    QTimer *m_autoCycleTimer;      // 10秒工作定时器
    QTimer *m_observationTimer;    // 2秒观察定时器 (暂停后等待数据刷新)

    bool m_isInObservationMode = false; // 标记当前是否处于"暂停观察"阶段

    // 【新增】功放控制：双路长连，写频开/关时对两路都发开/关指令
    QTcpSocket *m_ampSocket;
    QTcpSocket *m_ampSocket2;
    NetConfig m_currAmpCfg;
    NetConfig m_currAmp2Cfg;

signals:
    void sigLogMessage(const QString &msg);
    void sigDroneList(const QList<DroneInfo> &drones);
    void sigImageList(const QList<ImageInfo> &images);
    void sigAlertCount(int count);
    void sigSelfPosition(double lat, double lng);
    void sigTargetsUpdated(const QList<DroneInfo> &drones);
};

#endif // DEVICEMANAGER_H
