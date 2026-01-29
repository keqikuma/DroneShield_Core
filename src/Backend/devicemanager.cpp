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

    // 3. 侦测 (TCP Server)
    // 注意：这里逻辑变了，本机变成了 TCP 服务端
    m_detectionDriver = new DetectionDriver(this);

    // 连接侦测信号 (对接 DataStructs.h 中的结构)
    connect(m_detectionDriver, &DetectionDriver::sigDroneListUpdated,
            this, &DeviceManager::onDroneListUpdated);

    connect(m_detectionDriver, &DetectionDriver::sigImageListUpdated,
            this, &DeviceManager::onImageListUpdated);

    connect(m_detectionDriver, &DetectionDriver::sigAlertCountUpdated,
            this, &DeviceManager::onAlertCountUpdated);

    connect(m_detectionDriver, &DetectionDriver::sigDevicePositionUpdated,
            this, &DeviceManager::onDevicePositionUpdated);

    // 开启本地 TCP 监听，等待 Linux 主动连接推送数据
    // 端口 8089 是我们通过 Python 脚本验证过的
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
// 2. 侦测数据处理与转发 (新增部分)
// ============================================================================

void DeviceManager::onDroneListUpdated(const QList<DroneInfo> &drones)
{
    // 1. 转发全量数据给 UI 显示
    emit sigDroneList(drones);

    // 2. 兼容旧 UI 的信号 (如果还有旧代码依赖它)
    emit sigTargetsUpdated(drones);

    // 3. 为自动防御逻辑提取关键信息
    bool hasThreat = false;
    double minDistance = 999999.0;

    if (!drones.isEmpty()) {
        for (const auto &d : drones) {
            // 过滤白名单 (白名单无人机不触发防御)
            if (d.whiteList) continue;

            hasThreat = true;
            if (d.distance < minDistance) {
                minDistance = d.distance;
            }
        }
    }

    // 执行自动决策
    processDecision(hasThreat, minDistance);
}

void DeviceManager::onImageListUpdated(const QList<ImageInfo> &images)
{
    // 转发图传/频谱数据给 UI
    emit sigImageList(images);
}

void DeviceManager::onAlertCountUpdated(int count)
{
    // 转发告警数量给 UI (右上角红点)
    emit sigAlertCount(count);
}

void DeviceManager::onDevicePositionUpdated(double lat, double lng)
{
    // 转发设备自身位置给 UI (雷达中心点)
    emit sigSelfPosition(lat, lng);
}

// ============================================================================
// 3. 模式切换与总控
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
// 4. 自动模式决策 (核心业务逻辑)
// ============================================================================

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
// 5. 手动模式业务
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
