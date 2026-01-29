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

    // =========================================================
    // 【新增】初始化防抖定时器
    // =========================================================
    m_stopDefenseTimer = new QTimer(this);
    m_stopDefenseTimer->setInterval(3000); // 延时 3 秒关闭
    m_stopDefenseTimer->setSingleShot(true); // 只触发一次
    connect(m_stopDefenseTimer, &QTimer::timeout, this, &DeviceManager::onStopDefenseTimeout);

    ConfigLoader config;

    // 1. 诱骗 (UDP)
    QString spoofTargetIp = "192.168.10.230"; // 你的硬件IP
    int spoofTargetPort = 9099;
    m_spoofDriver = new SpoofDriver(spoofTargetIp, spoofTargetPort, this);
    connect(m_spoofDriver, &SpoofDriver::sigSpoofLog, this, &DeviceManager::sigLogMessage);

    // 2. 干扰 (HTTP)
    m_jammerDriver = new JammerDriver(this);
    m_jammerDriver->setTarget(Config::LINUX_MAIN_IP, Config::LINUX_PORT);

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

    log("[DeviceManager] 就绪");
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
    // 转发数据
    emit sigDroneList(drones);
    emit sigTargetsUpdated(drones);

    // 处理空列表 (信号丢失)
    if (drones.isEmpty()) {
        processDecision(false, 0);
        return;
    }

    bool hasThreat = false;
    double minDistance = 999999.0;

    for (const auto &d : drones) {
        // 过滤白名单
        if (d.whiteList) continue;

        hasThreat = true;
        if (d.distance < minDistance) {
            minDistance = d.distance;
        }
    }

    // 执行决策
    processDecision(hasThreat, minDistance);
}

// (onImageListUpdated 等其他函数保持不变...)
void DeviceManager::onImageListUpdated(const QList<ImageInfo> &images) { emit sigImageList(images); }
void DeviceManager::onAlertCountUpdated(int count) { emit sigAlertCount(count); }
void DeviceManager::onDevicePositionUpdated(double lat, double lng) { emit sigSelfPosition(lat, lng); }


// ============================================================================
// 3. 模式切换与总控
// ============================================================================

void DeviceManager::setSystemMode(SystemMode mode)
{
    if (m_currentMode == mode) return;

    // 如果切回手动，务必取消所有自动定时器
    if (mode == SystemMode::Manual) {
        m_stopDefenseTimer->stop();
    }

    m_currentMode = mode;
    log(QString("[DeviceManager] 切换模式 -> %1").arg(mode == SystemMode::Auto ? "自动" : "手动"));
    stopAllBusiness();
}

void DeviceManager::stopAllBusiness()
{
    // 【新增】停止业务时，也顺便把倒计时停了，防止它稍后误触发
    m_stopDefenseTimer->stop();

    // 1. 关诱骗
    if (m_spoofDriver) m_spoofDriver->setSwitch(false);
    m_isAutoSpoofingRunning = false;

    // 2. 关干扰
    if (m_jammerDriver) m_jammerDriver->setJamming(false);
    m_isLinuxJammerRunning = false;

    // 3. 关压制
    if (m_relayDriver) m_relayDriver->setAll(false);
    m_isRelaySuppressionRunning = false;

    log(">>> 所有设备复位 (OFF)");
}

// 【新增】防抖超时处理：真正停火的地方
void DeviceManager::onStopDefenseTimeout()
{
    if (m_currentMode != SystemMode::Auto) return;

    log("[自动决策] 信号丢失超过3秒 -> 停止防御");
    stopAllBusiness();
}

// ============================================================================
// 4. 自动模式决策 (含防抖逻辑)
// ============================================================================

void DeviceManager::processDecision(bool hasDrone, double distance)
{
    if (m_currentMode != SystemMode::Auto) return;

    // ---------------------------------------------------
    // A. 目标消失 -> 启动防抖倒计时
    // ---------------------------------------------------
    if (!hasDrone) {
        // 只有当防御正在运行，且还没开始倒计时的时候，才启动倒计时
        if ((m_isAutoSpoofingRunning || m_isRelaySuppressionRunning) && !m_stopDefenseTimer->isActive()) {
            log("[自动决策] 目标消失 -> 启动3秒防抖延时...");
            m_stopDefenseTimer->start(); // 3秒后触发 onStopDefenseTimeout
        }
        return; // 直接返回，暂不停火
    }

    // ---------------------------------------------------
    // A+. 目标重现 -> 取消防抖
    // ---------------------------------------------------
    if (m_stopDefenseTimer->isActive()) {
        log("[自动决策] 目标重现 -> 保持防御 (取消停止)");
        m_stopDefenseTimer->stop(); // 只要有数据，就掐断倒计时
    }

    // ---------------------------------------------------
    // B. 发现目标 -> 启动基础防御 (诱骗: 圆周)
    // ---------------------------------------------------
    if (!m_isAutoSpoofingRunning) {
        log(QString("[自动决策] 发现威胁 (%1m) -> 启动诱骗(圆周)").arg(distance, 0, 'f', 0));

        m_spoofDriver->setPosition(Config::BASE_LON, Config::BASE_LAT, 0);
        m_spoofDriver->setSwitch(true);
        m_spoofDriver->startCircular(500, 50);

        m_isAutoSpoofingRunning = true;
    }

    // ---------------------------------------------------
    // C. 距离分级处理 (压制)
    // ---------------------------------------------------
    if (distance <= 1000.0) {
        // [红区] < 1000m : 开启压制
        if (!m_isRelaySuppressionRunning) {
            log(QString("[自动决策] 进入红区 (%1m) -> 开启压制").arg(distance, 0, 'f', 0));
            if (m_relayDriver) m_relayDriver->setAll(true);
            m_isRelaySuppressionRunning = true;
        }
    }
    else {
        // [黄区] > 1000m : 关闭压制 (压制可以立即关，也可以跟防抖走，这里保持原样立即关)
        if (m_isRelaySuppressionRunning) {
            log("[自动决策] 离开红区 -> 停止压制");
            if (m_relayDriver) m_relayDriver->setAll(false);
            m_isRelaySuppressionRunning = false;
        }
    }
}

// (手动模式代码保持不变，此处省略...)
void DeviceManager::setManualSpoofSwitch(bool enable) { if(m_spoofDriver) m_spoofDriver->setSwitch(enable); }
void DeviceManager::setManualCircular() { m_spoofDriver->setPosition(Config::BASE_LON, Config::BASE_LAT, 0); m_spoofDriver->setSwitch(true); m_spoofDriver->startCircular(100, 50); }
void DeviceManager::setManualDirection(SpoofDirection dir) { m_spoofDriver->setPosition(Config::BASE_LON, Config::BASE_LAT, 0); m_spoofDriver->setSwitch(true); m_spoofDriver->startDirectional(dir, 15.0); }
void DeviceManager::setJammerConfig(const QList<JammerConfigData> &configs) { if(m_jammerDriver) m_jammerDriver->setWriteFreq(configs); }
void DeviceManager::setManualJammer(bool enable) { if(m_jammerDriver) m_jammerDriver->setJamming(enable); }
void DeviceManager::setRelayChannel(int channel, bool on) { if(m_relayDriver) m_relayDriver->setChannel(channel, on); }
void DeviceManager::setRelayAll(bool on) { if(m_relayDriver) m_relayDriver->setAll(on); }
