#include "devicemanager.h"
#include "../Utils/configloader.h"
#include "Consts.h"

DeviceManager::DeviceManager(QObject *parent) : QObject(parent)
{
    log("[DeviceManager] 系统初始化中...");

    m_currentMode = SystemMode::Manual;
    m_isAutoSpoofingRunning = false;
    m_isJammingRunning = false;

    ConfigLoader config;

    // 1. 初始化诱骗驱动
    m_spoofDriver = new SpoofDriver(config.getSpoofIp(), config.getSpoofPort(), this);

    // 2. 初始化干扰驱动
    m_jammerDriver = new JammerDriver(this);
    m_jammerDriver->setTarget(Config::LINUX_MAIN_IP, Config::LINUX_PORT);

    // 3. 初始化侦测驱动
    m_detectionDriver = new DetectionDriver(this);

    // 连接侦测信号
    connect(m_detectionDriver, &DetectionDriver::droneDetected,
            this, &DeviceManager::onRealTimeDroneDetected);
    connect(m_detectionDriver, &DetectionDriver::imageDetected,
            this, &DeviceManager::onRealTimeImageDetected);

    // 4. 启动连接
    m_detectionDriver->connectToDevice(Config::LINUX_MAIN_IP, Config::LINUX_PORT);

    // 预设写频
    log("[DeviceManager] 下发预设干扰频率参数...");
    m_jammerDriver->setWriteFreq(900, 920);
}

DeviceManager::~DeviceManager()
{
}

// === 辅助日志函数 ===
void DeviceManager::log(const QString &msg)
{
    qDebug() << msg;         // 打印到控制台
    emit sigLogMessage(msg); // 发送到 UI
}

// === 基础控制 ===
void DeviceManager::startSpoofing(double lat, double lon, double alt)
{
    log("[DeviceManager] 开启诱骗基础流程");
    if (m_spoofDriver) {
        m_spoofDriver->startSpoofing(lat, lon, alt);
    }
}

void DeviceManager::stopSpoofing()
{
    log("[DeviceManager] 停止业务 (诱骗 & 干扰)");

    if (m_spoofDriver) m_spoofDriver->stopSpoofing();
    m_isAutoSpoofingRunning = false;

    if (m_isJammingRunning && m_jammerDriver) {
        log("[DeviceManager] 联动停止干扰");
        m_jammerDriver->setJamming(false);
        m_isJammingRunning = false;
    }
}

// === 手动模式业务 ===
void DeviceManager::setSystemMode(SystemMode mode)
{
    if (m_currentMode == mode) return;

    m_currentMode = mode;
    log(QString("[DeviceManager] 切换模式 -> %1").arg(mode == SystemMode::Auto ? "自动" : "手动"));

    // 切换模式时停止当前动作
    stopSpoofing();
}

void DeviceManager::setManualCircular()
{
    if (m_currentMode != SystemMode::Manual) {
        qWarning() << "拒绝操作：当前不是手动模式";
        return;
    }
    log("[手动业务] 执行圆周驱离");
    if (m_spoofDriver) m_spoofDriver->setCircularMotion(500.0, 60.0);
}

void DeviceManager::setManualDirection(SpoofDirection dir)
{
    if (m_currentMode != SystemMode::Manual) {
        qWarning() << "拒绝操作：当前不是手动模式";
        return;
    }

    double angle = 0.0;
    QString dirName = "Unknown";
    switch (dir) {
    case SpoofDirection::North: angle = 0.0;   dirName = "北"; break;
    case SpoofDirection::East:  angle = 90.0;  dirName = "东"; break;
    case SpoofDirection::South: angle = 180.0; dirName = "南"; break;
    case SpoofDirection::West:  angle = 270.0; dirName = "西"; break;
    }

    log(QString("[手动业务] 执行定向驱离 -> 方向: %1").arg(dirName));
    if (m_spoofDriver) m_spoofDriver->setLinearMotion(15.0, angle);
}

// === 自动模式 / 实时侦测响应 ===
void DeviceManager::onRealTimeDroneDetected(const QList<DroneInfo> &drones)
{
    // 1. 立即把数据发给 UI 显示
    emit sigTargetsUpdated(drones);

    bool hasDrone = !drones.isEmpty();
    double distance = 0.0;

    if (hasDrone) {
        // 模拟计算距离 (实际项目中请用经纬度计算)
        distance = 800.0;
    }

    // 2. 进入决策流程
    processDecision(hasDrone, distance);
}

void DeviceManager::onRealTimeImageDetected(int count, const QString &desc)
{
    if (count > 0) {
        // 可选：如果只有图传也想触发打击，可以在这里调用 processDecision
    }
}

void DeviceManager::processDecision(bool hasDrone, double distance)
{
    if (m_currentMode != SystemMode::Auto) return;

    // A. 目标消失
    if (!hasDrone) {
        if (m_isAutoSpoofingRunning || m_isJammingRunning) {
            log("[自动决策] 目标消失 -> 全系统停机");
            stopSpoofing();
        }
        return;
    }

    // B. 发现目标 -> 启动诱骗
    if (!m_isAutoSpoofingRunning) {
        log("[自动决策] 发现威胁 -> 启动诱骗防御");
        startSpoofing(40.0, 116.0);
        m_isAutoSpoofingRunning = true;
    }

    // C. 战术动作
    if (m_spoofDriver) m_spoofDriver->setCircularMotion(500.0, 60.0);

    // D. 红区干扰判断
    if (distance <= 1000.0) {
        if (!m_isJammingRunning) {
            log(QString("[自动决策] !!! 进入红区 (%1米) !!! -> 开启干扰").arg(distance, 0, 'f', 1));
            if (m_jammerDriver) m_jammerDriver->setJamming(true);
            m_isJammingRunning = true;
        }
    } else {
        if (m_isJammingRunning) {
            log("[自动决策] 离开红区 -> 停止干扰");
            if (m_jammerDriver) m_jammerDriver->setJamming(false);
            m_isJammingRunning = false;
        }
    }
}
