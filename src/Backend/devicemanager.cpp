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
    m_isLinuxJammerRunning = false;
    m_isRelaySuppressionRunning = false;

    ConfigLoader config;

    // 1. 诱骗 (UDP)
    m_spoofDriver = new SpoofDriver(config.getSpoofIp(), config.getSpoofPort(), this);

    // 2. 干扰 (HTTP)
    m_jammerDriver = new JammerDriver(this);
    m_jammerDriver->setTarget(Config::LINUX_MAIN_IP, Config::LINUX_PORT);

    // 3. 侦测 (SocketIO)
    m_detectionDriver = new DetectionDriver(this);
    connect(m_detectionDriver, &DetectionDriver::droneDetected,
            this, &DeviceManager::onRealTimeDroneDetected);
    connect(m_detectionDriver, &DetectionDriver::imageDetected,
            this, &DeviceManager::onRealTimeImageDetected);
    m_detectionDriver->connectToDevice(Config::LINUX_MAIN_IP, Config::LINUX_PORT);

    // 4. 压制 (Relay TCP)
    m_relayDriver = new RelayDriver(this);
    m_relayDriver->connectToDevice(Config::RELAY_IP, Config::RELAY_PORT);

    // 5. 看门狗
    m_monitorTimer = new QTimer(this);
    m_monitorTimer->setInterval(5000);
    connect(m_monitorTimer, &QTimer::timeout, this, [this](){
        m_detectionDriver->connectToDevice(Config::LINUX_MAIN_IP, Config::LINUX_PORT);
    });
    m_monitorTimer->start();

    log("[DeviceManager] 就绪");
}

DeviceManager::~DeviceManager() {}

void DeviceManager::log(const QString &msg) {
    qDebug() << msg;
    emit sigLogMessage(msg);
}

// ============================================================================
// 2. 模式切换与总控
// ============================================================================

void DeviceManager::setSystemMode(SystemMode mode)
{
    if (m_currentMode == mode) return;
    m_currentMode = mode;
    log(QString("[DeviceManager] 切换模式 -> %1").arg(mode == SystemMode::Auto ? "自动" : "手动"));
    stopAllBusiness();
}

void DeviceManager::stopAllBusiness()
{
    // 1. 关诱骗
    if (m_spoofDriver) {
        m_spoofDriver->setSwitch(false);
    }
    m_isAutoSpoofingRunning = false;

    // 2. 关干扰
    if (m_jammerDriver) {
        m_jammerDriver->setJamming(false);
    }
    m_isLinuxJammerRunning = false;

    // 3. 关压制
    if (m_relayDriver) {
        m_relayDriver->setAll(false);
    }
    m_isRelaySuppressionRunning = false;

    log(">>> 所有设备复位 (OFF)");
}

// ============================================================================
// 3. 自动模式 (核心业务逻辑)
// ============================================================================

void DeviceManager::onRealTimeDroneDetected(const QList<DroneInfo> &drones)
{
    emit sigTargetsUpdated(drones); // 刷新UI

    bool hasDrone = !drones.isEmpty();
    double minDistance = 999999.0;
    if (hasDrone) {
        for (const auto &d : drones) {
            if (d.distance < minDistance) minDistance = d.distance;
        }
    }

    processDecision(hasDrone, minDistance);
}

void DeviceManager::onRealTimeImageDetected(int count, const QString &desc) {
    // 图传检测暂不触发动作
}

void DeviceManager::processDecision(bool hasDrone, double distance)
{
    if (m_currentMode != SystemMode::Auto) return;

    // ---------------------------------------------------
    // A. 目标消失 -> 停一切
    // ---------------------------------------------------
    if (!hasDrone) {
        if (m_isAutoSpoofingRunning || m_isRelaySuppressionRunning) {
            log("[自动决策] 目标消失 -> 停止防御");
            stopAllBusiness();
        }
        return;
    }

    // ---------------------------------------------------
    // B. 发现目标 -> 启动基础防御 (诱骗: 圆周)
    // ---------------------------------------------------
    if (!m_isAutoSpoofingRunning) {
        log(QString("[自动决策] 发现威胁 (%1m) -> 启动诱骗(圆周)").arg(distance, 0, 'f', 0));

        // 1. 设置位置 (使用基站坐标)
        m_spoofDriver->setPosition(Config::BASE_LON, Config::BASE_LAT, 0);
        // 2. 开开关
        m_spoofDriver->setSwitch(true);
        // 3. 设模式 (圆周: 半径500)
        m_spoofDriver->startCircular(500, 50);

        m_isAutoSpoofingRunning = true;
    }

    // ---------------------------------------------------
    // C. 距离分级处理
    // ---------------------------------------------------
    if (distance <= 1000.0) {
        // [红区] < 1000m : 保持诱骗 + 开启压制
        if (!m_isRelaySuppressionRunning) {
            log(QString("[自动决策] !!! 进入红区 (%1m) !!! -> 开启压制").arg(distance, 0, 'f', 0));
            if (m_relayDriver) m_relayDriver->setAll(true);
            m_isRelaySuppressionRunning = true;
        }
    }
    else {
        // [黄区] > 1000m : 保持诱骗 + 关闭压制
        if (m_isRelaySuppressionRunning) {
            log("[自动决策] 目标离开红区 -> 停止压制");
            if (m_relayDriver) m_relayDriver->setAll(false);
            m_isRelaySuppressionRunning = false;
        }
    }
}

// ============================================================================
// 4. 手动模式业务
// ============================================================================

// --- 诱骗 ---
void DeviceManager::setManualSpoofSwitch(bool enable)
{
    if (m_currentMode != SystemMode::Manual) return;
    // 仅开关射频
    if (m_spoofDriver) m_spoofDriver->setSwitch(enable);
}

void DeviceManager::setManualCircular()
{
    if (m_currentMode != SystemMode::Manual) {
        qWarning() << "非手动模式";
        return;
    }
    log("[手动指令] 执行圆周驱离");

    // 手动执行的标准序列: 位置 -> 开关 -> 模式
    m_spoofDriver->setPosition(Config::BASE_LON, Config::BASE_LAT, 0);
    m_spoofDriver->setSwitch(true);
    m_spoofDriver->startCircular(100, 50); // 手动默认参数
}

void DeviceManager::setManualDirection(SpoofDirection dir)
{
    if (m_currentMode != SystemMode::Manual) return;

    QString dName;
    if(dir == SpoofDirection::North) dName = "北";
    if(dir == SpoofDirection::East) dName = "东";
    if(dir == SpoofDirection::South) dName = "南";
    if(dir == SpoofDirection::West) dName = "西";

    log(QString("[手动指令] 执行定向驱离 -> %1").arg(dName));

    m_spoofDriver->setPosition(Config::BASE_LON, Config::BASE_LAT, 0);
    m_spoofDriver->setSwitch(true);
    m_spoofDriver->startDirectional(dir, 15.0); // 速度15m/s
}

// --- 干扰 (Linux) ---
void DeviceManager::setJammerConfig(const QList<JammerConfigData> &configs) {
    if (m_jammerDriver) m_jammerDriver->setWriteFreq(configs);
}

void DeviceManager::setManualJammer(bool enable) {
    log(QString("[手动指令] 干扰开关 -> %1").arg(enable ? "ON" : "OFF"));
    if (m_jammerDriver) m_jammerDriver->setJamming(enable);
    m_isLinuxJammerRunning = enable;
}

// --- 压制 (Relay) ---
void DeviceManager::setRelayChannel(int channel, bool on) {
    if (m_relayDriver) m_relayDriver->setChannel(channel, on);
}

void DeviceManager::setRelayAll(bool on) {
    if (m_relayDriver) m_relayDriver->setAll(on);
    if (m_currentMode == SystemMode::Auto) m_isRelaySuppressionRunning = on;
}
