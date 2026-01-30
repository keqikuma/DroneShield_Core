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

    // 防抖定时器 (3秒)
    m_stopDefenseTimer = new QTimer(this);
    m_stopDefenseTimer->setInterval(10000);
    m_stopDefenseTimer->setSingleShot(true);
    connect(m_stopDefenseTimer, &QTimer::timeout, this, &DeviceManager::onStopDefenseTimeout);

    ConfigLoader config;

    // 1. 诱骗 (UDP)
    QString spoofTargetIp = "192.168.10.230";
    int spoofTargetPort = 9099;
    m_spoofDriver = new SpoofDriver(spoofTargetIp, spoofTargetPort, this);
    connect(m_spoofDriver, &SpoofDriver::sigSpoofLog, this, &DeviceManager::sigLogMessage);

    // 2. 干扰 (HTTP)
    m_jammerDriver = new JammerDriver(this);
    m_jammerDriver->setTarget("192.178.1.12", 8090); // 确认端口是 8090

    // 【新增】连接日志信号！这样 JammerDriver 发出的 emit sigLog 就会显示在界面上
    // 假设你之前已经有了 sigLogMessage 的处理逻辑
    connect(m_jammerDriver, &JammerDriver::sigLog, this, &DeviceManager::sigLogMessage);

    // 3. 侦测 (TCP Server)
    m_detectionDriver = new DetectionDriver(this);
    connect(m_detectionDriver, &DetectionDriver::sigDroneListUpdated, this, &DeviceManager::onDroneListUpdated);
    connect(m_detectionDriver, &DetectionDriver::sigImageListUpdated, this, &DeviceManager::onImageListUpdated);
    connect(m_detectionDriver, &DetectionDriver::sigAlertCountUpdated, this, &DeviceManager::onAlertCountUpdated);
    connect(m_detectionDriver, &DetectionDriver::sigDevicePositionUpdated, this, &DeviceManager::onDevicePositionUpdated);
    m_detectionDriver->startServer(8089);

    // 4. 压制 (Relay TCP)
    m_relayDriver = new RelayDriver(this);
    m_relayDriver->connectToDevice(Config::RELAY_IP, Config::RELAY_PORT);

    log("[DeviceManager] 就绪 (诱骗目标: 192.168.10.230)");
}

DeviceManager::~DeviceManager() {}

void DeviceManager::log(const QString &msg) {
    qDebug() << msg;
    emit sigLogMessage(msg);
}

// ============================================================================
// 2. 侦测数据处理 (全源触发)
// ============================================================================

// --- A. 无人机协议数据 ---
void DeviceManager::onDroneListUpdated(const QList<DroneInfo> &drones)
{
    emit sigDroneList(drones);
    emit sigTargetsUpdated(drones);

    bool hasDroneThreat = false;
    double minDistance = 999999.0;

    if (!drones.isEmpty()) {
        for (const auto &d : drones) {
            // 白名单过滤 (默认false，即所有都是威胁)
            if (d.whiteList) continue;

            hasDroneThreat = true;

            // 【关键修改】距离清洗逻辑
            // 如果坐标是0，或者距离是0，说明没定位，强制设为超远距离
            // 这样就 只开诱骗，不开压制
            double realDist = d.distance;
            if (d.uav_lat == 0.0 && d.uav_lng == 0.0) {
                realDist = 999999.0;
            }
            if (realDist <= 0.1) {
                realDist = 999999.0;
            }

            if (realDist < minDistance) {
                minDistance = realDist;
            }
        }
    }

    // 综合判断：是否有无人机威胁 OR 是否有图传威胁
    // 注意：图传没有距离信息，默认给最大距离 (只诱骗，不压制)
    bool finalThreat = hasDroneThreat || m_hasImageThreat;

    // 只有当存在无人机协议时，才使用计算出的 minDistance
    // 如果只有图传威胁，距离就是无穷大
    double finalDistance = hasDroneThreat ? minDistance : 999999.0;

    processDecision(finalThreat, finalDistance);
}

// --- B. 图传/频谱数据 (新增触发源) ---
void DeviceManager::onImageListUpdated(const QList<ImageInfo> &images)
{
    emit sigImageList(images);

    // 只要列表不为空，就认为有威胁 (图传/FPV都算)
    m_hasImageThreat = !images.isEmpty();

    // 如果当前没有无人机协议数据（DroneList可能为空），这里也必须触发决策
    // 距离设为无穷大 -> 触发诱骗，但不触发压制
    if (m_hasImageThreat) {
        processDecision(true, 999999.0);
    }
    else {
        // 图传没了，但也得调用一下决策，万一无人机协议也没了，好让系统停机
        // 这里传入 false，让 processDecision 去检查是否真的要停
        // 注意：这里没法获取无人机协议的状态，所以这只是个辅助触发
        // 真正的停机主要靠 DroneList 那个空的包来触发，或者防抖超时
        // 简单起见，我们主要靠 DroneList 驱动，这里只负责“有威胁”时的即时触发
    }
}

void DeviceManager::onAlertCountUpdated(int count) { emit sigAlertCount(count); }
void DeviceManager::onDevicePositionUpdated(double lat, double lng) { emit sigSelfPosition(lat, lng); }

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

    // 重置威胁状态
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
// 4. 自动模式决策 (严格按你要求的顺序)
// ============================================================================

void DeviceManager::processDecision(bool hasThreat, double distance)
{
    if (m_currentMode != SystemMode::Auto) return;

    // ----------------------------------------------------------------
    // 场景 1: 目标消失 (或信号丢失)
    // ----------------------------------------------------------------
    if (!hasThreat) {
        // 正在运行 && 没在倒计时 -> 启动倒计时
        if ((m_isAutoSpoofingRunning || m_isRelaySuppressionRunning) && !m_stopDefenseTimer->isActive()) {
            log("[自动决策] 目标消失 -> 启动3秒防抖延时...");
            m_stopDefenseTimer->start();
        }
        return;
    }

    // ----------------------------------------------------------------
    // 场景 2: 目标存在 (重置倒计时)
    // ----------------------------------------------------------------
    if (m_stopDefenseTimer->isActive()) {
        m_stopDefenseTimer->stop(); // 续命
    }

    // ----------------------------------------------------------------
    // 场景 3: 触发诱骗 (第一优先级)
    // 只要有威胁，不管距离多少，先开诱骗
    // ----------------------------------------------------------------
    if (!m_isAutoSpoofingRunning) {
        log("[自动决策] 发现威胁 -> 启动诱骗(圆周)");

        // 严格按照：设坐标 -> 开开关 -> 设模式
        m_spoofDriver->setPosition(Config::BASE_LON, Config::BASE_LAT, 0);
        m_spoofDriver->setSwitch(true);
        m_spoofDriver->startCircular(500, 50); // 半径500，速度50

        m_isAutoSpoofingRunning = true;
    }

    // ----------------------------------------------------------------
    // 场景 4: 触发压制 (第二优先级，看距离)
    // ----------------------------------------------------------------
    if (distance <= 1000.0) {
        // [红区] < 1000m : 必须开启压制
        if (!m_isRelaySuppressionRunning) {
            log(QString("[自动决策] 进入红区 (%1m) -> 开启压制").arg(distance));
            if (m_relayDriver) m_relayDriver->setAll(true);
            m_isRelaySuppressionRunning = true;
        }
    }
    else {
        // [黄区] > 1000m : 必须关闭压制 (但保持诱骗)
        // 比如测试时没坐标，distance=999999，就会进到这里，只开诱骗
        if (m_isRelaySuppressionRunning) {
            log("[自动决策] 离开红区 -> 停止压制");
            if (m_relayDriver) m_relayDriver->setAll(false);
            m_isRelaySuppressionRunning = false;
        }
    }
}

// (手动模式代码保持不变)
void DeviceManager::setManualSpoofSwitch(bool enable) { if(m_spoofDriver) m_spoofDriver->setSwitch(enable); }
void DeviceManager::setManualCircular() { m_spoofDriver->setPosition(Config::BASE_LON, Config::BASE_LAT, 0); m_spoofDriver->setSwitch(true); m_spoofDriver->startCircular(100, 50); }
void DeviceManager::setManualDirection(SpoofDirection dir) { m_spoofDriver->setPosition(Config::BASE_LON, Config::BASE_LAT, 0); m_spoofDriver->setSwitch(true); m_spoofDriver->startDirectional(dir, 15.0); }
void DeviceManager::setJammerConfig(const QList<JammerConfigData> &configs) { if(m_jammerDriver) m_jammerDriver->setWriteFreq(configs); }
void DeviceManager::setManualJammer(bool enable) { if(m_jammerDriver) m_jammerDriver->setJamming(enable); }
void DeviceManager::setRelayChannel(int channel, bool on) { if(m_relayDriver) m_relayDriver->setChannel(channel, on); }
void DeviceManager::setRelayAll(bool on) { if(m_relayDriver) m_relayDriver->setAll(on); }
