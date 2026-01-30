#include "devicemanager.h"
#include "../Utils/configloader.h"
#include "Consts.h"

// ============================================================================
// 1. 初始化
// ============================================================================
DeviceManager::DeviceManager(QObject *parent) : QObject(parent)
{
    log("[DeviceManager] 系统核心初始化...");
    m_currentMode = SystemMode::Manual;
    m_isAutoSpoofingRunning = false;
    m_isRelaySuppressionRunning = false;
    m_hasImageThreat = false;

    // 初始化基站坐标默认值
    m_baseLat = Config::BASE_LAT;
    m_baseLng = Config::BASE_LON;

    // 防抖定时器 (3秒)
    m_stopDefenseTimer = new QTimer(this);
    m_stopDefenseTimer->setInterval(3000);
    m_stopDefenseTimer->setSingleShot(true);
    connect(m_stopDefenseTimer, &QTimer::timeout, this, &DeviceManager::onStopDefenseTimeout);

    ConfigLoader config;

    // 1. 诱骗 (UDP)
    QString spoofTargetIp = "192.168.10.230";
    int spoofTargetPort = 9099;
    m_spoofDriver = new SpoofDriver(spoofTargetIp, spoofTargetPort, this);
    connect(m_spoofDriver, &SpoofDriver::sigSpoofLog, this, &DeviceManager::sigLogMessage);

    // 【连接诱骗坐标】这是主要且准确的坐标源
    connect(m_spoofDriver, &SpoofDriver::sigDevicePosition, this, &DeviceManager::onDevicePositionUpdated);

    // 2. 干扰 (HTTP)
    m_jammerDriver = new JammerDriver(this);
    m_jammerDriver->setTarget("192.178.1.12", 8090);
    connect(m_jammerDriver, &JammerDriver::sigLog, this, &DeviceManager::sigLogMessage);

    // 3. 侦测 (WebSocket)
    m_detectionDriver = new DetectionDriver(this);

    // 连接信号
    connect(m_detectionDriver, &DetectionDriver::sigDroneListUpdated,
            this, &DeviceManager::onDroneListUpdated);
    connect(m_detectionDriver, &DetectionDriver::sigImageListUpdated,
            this, &DeviceManager::onImageListUpdated);

    // ==========================================================================
    // 【关键修改 1】已删除侦测系统的坐标连接！
    // 之前这里连了 sigDevicePositionUpdated，导致侦测发来的 0.0 会覆盖诱骗的正确坐标
    // ==========================================================================

    connect(m_detectionDriver, &DetectionDriver::sigLog,
            this, &DeviceManager::sigLogMessage);

    // 启动 WebSocket 连接
    QString wsUrl = "ws://192.178.1.12:8090/socket.io/?EIO=3&transport=websocket";
    m_detectionDriver->startWork(wsUrl);

    // 4. 压制 (Relay TCP)
    m_relayDriver = new RelayDriver(this);

    // 连接日志，这样你就能看到 "[压制] TCP 连接成功" 了
    connect(m_relayDriver, &RelayDriver::sigLog, this, &DeviceManager::sigLogMessage);

    // 使用你提供的 IP 和 端口
    // 192.168.10.221 : 4196
    m_relayDriver->connectToDevice("192.168.10.221", 4196);

    log("[DeviceManager] 就绪 (诱骗目标: 192.168.10.230)");
}

DeviceManager::~DeviceManager() {}

void DeviceManager::log(const QString &msg) {
    qDebug() << msg;
    emit sigLogMessage(msg);
}

// ============================================================================
// 2. 侦测数据处理
// ============================================================================

void DeviceManager::onDroneListUpdated(const QList<DroneInfo> &drones)
{
    emit sigDroneList(drones);
    emit sigTargetsUpdated(drones);

    bool hasDroneThreat = false;
    double minDistance = 999999.0;

    if (!drones.isEmpty()) {
        for (const auto &d : drones) {
            if (d.whiteList) continue;
            hasDroneThreat = true;

            // 距离清洗逻辑
            double realDist = d.distance;
            if ((d.uav_lat == 0.0 && d.uav_lng == 0.0) || realDist <= 0.1) {
                realDist = 999999.0;
            }

            if (realDist < minDistance) {
                minDistance = realDist;
            }
        }
    }

    bool finalThreat = hasDroneThreat || m_hasImageThreat;
    double finalDistance = hasDroneThreat ? minDistance : 999999.0;

    processDecision(finalThreat, finalDistance);
}

void DeviceManager::onImageListUpdated(const QList<ImageInfo> &images)
{
    emit sigImageList(images);
    m_hasImageThreat = !images.isEmpty();

    if (m_hasImageThreat) {
        processDecision(true, 999999.0);
    }
}

void DeviceManager::onAlertCountUpdated(int count) { emit sigAlertCount(count); }

// ============================================================================
// 【关键修改 2】坐标更新函数
// ============================================================================
void DeviceManager::onDevicePositionUpdated(double lat, double lng)
{
    // 过滤无效坐标：如果收到 0 或极小值，直接忽略，保持上一次的有效值
    if (lat < 0.1 || lng < 0.1) {
        return;
    }

    // 更新内部缓存
    m_baseLat = lat;
    m_baseLng = lng;

    // 转发给 UI 显示
    emit sigSelfPosition(lat, lng);
}

// ============================================================================
// 3. 模式切换与总控
// ============================================================================

void DeviceManager::setSystemMode(SystemMode mode)
{
    if (m_currentMode == mode) return;

    if (mode == SystemMode::Manual) {
        m_stopDefenseTimer->stop();
    }

    m_currentMode = mode;
    log(QString("[DeviceManager] 切换模式 -> %1").arg(mode == SystemMode::Auto ? "自动" : "手动"));
    stopAllBusiness();
}

void DeviceManager::stopAllBusiness()
{
    m_stopDefenseTimer->stop();

    if (m_spoofDriver) m_spoofDriver->setSwitch(false);
    m_isAutoSpoofingRunning = false;

    if (m_jammerDriver) m_jammerDriver->setJamming(false);

    if (m_relayDriver) m_relayDriver->setAll(false);
    m_isRelaySuppressionRunning = false;

    m_hasImageThreat = false;

    log(">>> 所有设备复位 (OFF)");
}

void DeviceManager::onStopDefenseTimeout()
{
    if (m_currentMode != SystemMode::Auto) return;
    log("[自动决策] 信号丢失超过3秒 -> 停止防御");
    stopAllBusiness();
}

// ============================================================================
// 4. 自动模式决策
// ============================================================================

void DeviceManager::processDecision(bool hasThreat, double distance)
{
    if (m_currentMode != SystemMode::Auto) return;

    // 目标消失
    if (!hasThreat) {
        if ((m_isAutoSpoofingRunning || m_isRelaySuppressionRunning) && !m_stopDefenseTimer->isActive()) {
            log("[自动决策] 目标消失 -> 启动3秒防抖延时...");
            m_stopDefenseTimer->start();
        }
        return;
    }

    // 目标存在
    if (m_stopDefenseTimer->isActive()) {
        m_stopDefenseTimer->stop();
    }

    // 触发诱骗
    if (!m_isAutoSpoofingRunning) {
        log("[自动决策] 发现威胁 -> 启动诱骗(圆周)");

        double targetLat = m_baseLat;
        double targetLng = m_baseLng;

        if (targetLat < 1.0 || targetLng < 1.0) {
            targetLat = Config::BASE_LAT;
            targetLng = Config::BASE_LON;
            log("[自动决策] 暂未获取基站坐标，使用默认配置");
        } else {
            log(QString("[自动决策] 使用基站实测坐标: %1, %2").arg(targetLat).arg(targetLng));
        }

        m_spoofDriver->setPosition(targetLng, targetLat, 0);
        m_spoofDriver->setSwitch(true);
        m_spoofDriver->startCircular(500, 50);

        m_isAutoSpoofingRunning = true;
    }

    // 触发压制
    if (distance <= 1000.0) {
        if (!m_isRelaySuppressionRunning) {
            log(QString("[自动决策] 进入红区 (%1m) -> 开启压制").arg(distance));
            if (m_relayDriver) m_relayDriver->setAll(true);
            m_isRelaySuppressionRunning = true;
        }
    }
    else {
        if (m_isRelaySuppressionRunning) {
            log("[自动决策] 离开红区 -> 停止压制");
            if (m_relayDriver) m_relayDriver->setAll(false);
            m_isRelaySuppressionRunning = false;
        }
    }
}

// (手动模式代码)
void DeviceManager::setManualSpoofSwitch(bool enable) { if(m_spoofDriver) m_spoofDriver->setSwitch(enable); }
void DeviceManager::setManualCircular() { m_spoofDriver->setPosition(Config::BASE_LON, Config::BASE_LAT, 0); m_spoofDriver->setSwitch(true); m_spoofDriver->startCircular(100, 50); }
void DeviceManager::setManualDirection(SpoofDirection dir) { m_spoofDriver->setPosition(Config::BASE_LON, Config::BASE_LAT, 0); m_spoofDriver->setSwitch(true); m_spoofDriver->startDirectional(dir, 15.0); }
void DeviceManager::setJammerConfig(const QList<JammerConfigData> &configs) { if(m_jammerDriver) m_jammerDriver->setWriteFreq(configs); }
void DeviceManager::setManualJammer(bool enable) { if(m_jammerDriver) m_jammerDriver->setJamming(enable); }
void DeviceManager::setRelayChannel(int channel, bool on) { if(m_relayDriver) m_relayDriver->setChannel(channel, on); }
void DeviceManager::setRelayAll(bool on) { if(m_relayDriver) m_relayDriver->setAll(on); }
