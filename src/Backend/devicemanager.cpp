#include "devicemanager.h"
#include "../Utils/configloader.h"
#include "Consts.h"

// ============================================================================
// 1. 初始化与析构
// ============================================================================

DeviceManager::DeviceManager(QObject *parent) : QObject(parent)
{
    log("[DeviceManager] 系统核心初始化...");

    // 初始化状态标志
    m_currentMode = SystemMode::Manual;
    m_isAutoSpoofingRunning = false;
    m_isLinuxJammerRunning = false;      // 手动 Linux 干扰状态
    m_isRelaySuppressionRunning = false; // 自动 继电器压制状态

    ConfigLoader config;

    // 1. 初始化驱动：诱骗 (UDP)
    m_spoofDriver = new SpoofDriver(config.getSpoofIp(), config.getSpoofPort(), this);

    // 2. 初始化驱动：Linux 干扰 (HTTP) - 仅手动模式使用
    m_jammerDriver = new JammerDriver(this);
    m_jammerDriver->setTarget(Config::LINUX_MAIN_IP, Config::LINUX_PORT);

    // 3. 初始化驱动：侦测 (SocketIO)
    m_detectionDriver = new DetectionDriver(this);

    // 连接侦测信号
    connect(m_detectionDriver, &DetectionDriver::droneDetected,
            this, &DeviceManager::onRealTimeDroneDetected);
    connect(m_detectionDriver, &DetectionDriver::imageDetected,
            this, &DeviceManager::onRealTimeImageDetected);

    // 4. 启动连接 (侦测服务)
    m_detectionDriver->connectToDevice(Config::LINUX_MAIN_IP, Config::LINUX_PORT);

    // 5. 初始化驱动：继电器 (TCP) - 压制设备
    m_relayDriver = new RelayDriver(this);
    // 连接到设备 (模拟时连接本地 Python Mock 端口 2000)
    m_relayDriver->connectToDevice(Config::RELAY_IP, Config::RELAY_PORT);

    // 6. 启动系统监控定时器 (断线自动重连)
    m_monitorTimer = new QTimer(this);
    m_monitorTimer->setInterval(5000); // 每 5秒检查一次
    connect(m_monitorTimer, &QTimer::timeout, this, [this](){
        // 定时尝试连接 (SocketClient 内部会判断如果已连接则忽略)
        m_detectionDriver->connectToDevice(Config::LINUX_MAIN_IP, Config::LINUX_PORT);
    });
    m_monitorTimer->start();

    log("[DeviceManager] 初始化完成，等待指令...");
}

DeviceManager::~DeviceManager()
{
}

void DeviceManager::log(const QString &msg)
{
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

    // 切换模式时，为了安全，必须停止当前所有的业务
    stopAllBusiness();
}

void DeviceManager::stopAllBusiness()
{
    // 1. 停止诱骗
    if (m_isAutoSpoofingRunning) {
        stopSpoofing(); // 内部会处理 m_isAutoSpoofingRunning = false
    }

    // 2. 停止手动干扰 (Linux)
    if (m_isLinuxJammerRunning) {
        setManualJammer(false);
    }

    // 3. 停止压制 (Relay - 包含自动和手动触发的)
    if (m_relayDriver) {
        m_relayDriver->setAll(false); // 发送全关指令
    }
    if (m_isRelaySuppressionRunning) {
        log("[DeviceManager] 停止压制 (Relay)");
        m_isRelaySuppressionRunning = false;
    }
}

// ============================================================================
// 3. 手动模式业务
// ============================================================================

// --- Linux 干扰板卡 ---

void DeviceManager::setJammerConfig(const QList<JammerConfigData> &configs)
{
    // 任何模式下都允许配置参数
    log(QString("[手动干扰] 下发频率配置参数 (%1 组)").arg(configs.size()));
    if (m_jammerDriver) {
        m_jammerDriver->setWriteFreq(configs);
    }
}

void DeviceManager::setManualJammer(bool enable)
{
    // 这是 Linux 板卡的开关，完全由用户按钮控制
    log(QString("[手动干扰] Linux 干扰开关 -> %1").arg(enable ? "ON" : "OFF"));

    if (m_jammerDriver) {
        m_jammerDriver->setJamming(enable);
    }
    m_isLinuxJammerRunning = enable;
}

// --- 诱骗设备 ---

void DeviceManager::setManualCircular()
{
    if (m_currentMode != SystemMode::Manual) {
        qWarning() << "拒绝操作：当前不是手动模式";
        return;
    }
    log("[手动诱骗] 执行圆周驱离");
    if (m_spoofDriver) m_spoofDriver->setCircularMotion(500.0, 60.0);
}

void DeviceManager::setManualDirection(SpoofDirection dir)
{
    if (m_currentMode != SystemMode::Manual) return;

    double angle = 0.0;
    QString dirName = "Unknown";
    switch (dir) {
    case SpoofDirection::North: angle = 0.0;   dirName = "北"; break;
    case SpoofDirection::East:  angle = 90.0;  dirName = "东"; break;
    case SpoofDirection::South: angle = 180.0; dirName = "南"; break;
    case SpoofDirection::West:  angle = 270.0; dirName = "西"; break;
    }

    log(QString("[手动诱骗] 执行定向驱离 -> 方向: %1").arg(dirName));
    if (m_spoofDriver) m_spoofDriver->setLinearMotion(15.0, angle);
}

// --- 继电器压制 (新增) ---

void DeviceManager::setRelayChannel(int channel, bool on)
{
    // 允许手动控制单路继电器
    if (m_relayDriver) {
        m_relayDriver->setChannel(channel, on);
    }
}

void DeviceManager::setRelayAll(bool on)
{
    // 允许手动全开/全关
    if (m_relayDriver) {
        m_relayDriver->setAll(on);
    }

    // 如果当前是自动模式，更新状态锁，避免逻辑冲突
    if (m_currentMode == SystemMode::Auto) {
        m_isRelaySuppressionRunning = on;
    }
}

// ============================================================================
// 4. 自动模式业务 (自动决策核心)
// ============================================================================

void DeviceManager::onRealTimeDroneDetected(const QList<DroneInfo> &drones)
{
    // 1. 立即把数据发给 UI 显示 (前端只管显示，不参与逻辑)
    emit sigTargetsUpdated(drones);

    bool hasDrone = !drones.isEmpty();
    double minDistance = 999999.0;

    // 找到最近的一个威胁目标的距离
    if (hasDrone) {
        for (const auto &d : drones) {
            if (d.distance < minDistance) {
                minDistance = d.distance;
            }
        }
    }

    // 2. 进入自动决策流程
    processDecision(hasDrone, minDistance);
}

void DeviceManager::onRealTimeImageDetected(int count, const QString &desc)
{
    // 图传信号暂时只做日志，不触发自动打击 (可根据需求修改)
    if (count > 0) {
        // processDecision(true, 500.0); // 示例：如果发现图传，假定距离很近
    }
}

void DeviceManager::processDecision(bool hasDrone, double distance)
{
    // 如果不是自动模式，什么都不做 (除了显示数据)
    if (m_currentMode != SystemMode::Auto) return;

    // ---------------------------------------------------------
    // 场景 A: 目标消失 -> 停止所有自动动作
    // ---------------------------------------------------------
    if (!hasDrone) {
        if (m_isAutoSpoofingRunning || m_isRelaySuppressionRunning) {
            log("[自动决策] 目标消失 -> 停止自动防御");
            stopSpoofing(); // 停诱骗

            // 停压制 (Relay)
            if (m_isRelaySuppressionRunning) {
                if (m_relayDriver) m_relayDriver->setAll(false); // 全关指令
                m_isRelaySuppressionRunning = false;
                log("[自动决策] 停止压制 (Relay)");
            }
        }
        return;
    }

    // ---------------------------------------------------------
    // 场景 B: 发现目标 -> 启动基础防御 (诱骗: 圆周驱离)
    // ---------------------------------------------------------
    if (!m_isAutoSpoofingRunning) {
        log(QString("[自动决策] 发现威胁 (最近 %.0fm) -> 启动诱骗驱离").arg(distance));
        // 启动诱骗 (假设以当前发现位置或默认位置启动)
        // 实际中可能需要传入经纬度，这里演示用默认参数
        startSpoofing(Config::BASE_LAT + 0.01, Config::BASE_LON + 0.01);

        // 设置为圆周模式
        if (m_spoofDriver) m_spoofDriver->setCircularMotion(500.0, 60.0);

        m_isAutoSpoofingRunning = true;
    }

    // ---------------------------------------------------------
    // 场景 C: 进入红区 (< 1000m) -> 叠加压制 (继电器)
    // ---------------------------------------------------------
    if (distance <= 1000.0) {
        if (!m_isRelaySuppressionRunning) {
            log(QString("[自动决策] !!! 进入红区 (%.0f米) !!! -> 触发全频段压制 (Relay)").arg(distance));

            // 自动开启全开
            if (m_relayDriver) m_relayDriver->setAll(true);

            m_isRelaySuppressionRunning = true;
        }
    }
    else {
        // 目标飞出红区 -> 停止压制
        if (m_isRelaySuppressionRunning) {
            log("[自动决策] 目标离开红区 -> 停止压制 (Relay)");

            // 自动关闭
            if (m_relayDriver) m_relayDriver->setAll(false);

            m_isRelaySuppressionRunning = false;
        }
    }
}

// ============================================================================
// 5. 内部辅助函数
// ============================================================================

void DeviceManager::startSpoofing(double lat, double lon, double alt)
{
    if (m_spoofDriver) {
        m_spoofDriver->startSpoofing(lat, lon, alt);
    }
}

void DeviceManager::stopSpoofing()
{
    if (m_spoofDriver) {
        m_spoofDriver->stopSpoofing();
    }
    m_isAutoSpoofingRunning = false;
}
