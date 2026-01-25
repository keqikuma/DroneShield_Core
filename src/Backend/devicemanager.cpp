#include "devicemanager.h"
#include "../Utils/configloader.h"
#include "../Consts.h"

DeviceManager::DeviceManager(QObject *parent) : QObject(parent)
{
    qDebug() << "[DeviceManager] 系统初始化中...";

    m_currentMode = SystemMode::Manual;
    m_isAutoSpoofingRunning = false;
    m_isJammingRunning = false;

    ConfigLoader config;

    // 1. 初始化诱骗驱动 (UDP -> FPGA)
    m_spoofDriver = new SpoofDriver(config.getSpoofIp(), config.getSpoofPort(), this);

    // 2. 初始化干扰/写频驱动 (HTTP -> Linux)
    m_jammerDriver = new JammerDriver(this);
    // 设置 Linux 主控板 IP
    m_jammerDriver->setTarget(Config::LINUX_MAIN_IP, Config::LINUX_PORT);

    // 3. 初始化侦测驱动 (SocketIO <- Linux)
    m_detectionDriver = new DetectionDriver(this);

    // 连接侦测信号 -> 业务槽函数
    connect(m_detectionDriver, &DetectionDriver::droneDetected,
            this, &DeviceManager::onRealTimeDroneDetected);
    connect(m_detectionDriver, &DetectionDriver::imageDetected,
            this, &DeviceManager::onRealTimeImageDetected);

    // 4. 启动连接与初始化配置
    // 连接侦测 WebSocket
    m_detectionDriver->connectToDevice(Config::LINUX_MAIN_IP, Config::LINUX_PORT);

    // 预设干扰频率参数 (900-920MHz)
    // 注意：这里只是配置参数，不会开启干扰
    qDebug() << "[DeviceManager] 下发预设干扰频率参数...";
    m_jammerDriver->setWriteFreq(900, 920);
}

DeviceManager::~DeviceManager()
{
    // Qt 对象树会自动回收子对象
}

void DeviceManager::startSpoofing(double lat, double lon, double alt)
{
    qDebug() << "[DeviceManager] 开启诱骗基础流程";
    if (m_spoofDriver) {
        m_spoofDriver->startSpoofing(lat, lon, alt);
    }
}

void DeviceManager::stopSpoofing()
{
    qDebug() << "[DeviceManager] 停止业务 (诱骗 & 干扰)";

    // 1. 停止诱骗
    if (m_spoofDriver) m_spoofDriver->stopSpoofing();
    m_isAutoSpoofingRunning = false;

    // 2. 停止干扰 (如果在运行)
    if (m_isJammingRunning && m_jammerDriver) {
        qDebug() << "[DeviceManager] 联动停止干扰";
        m_jammerDriver->setJamming(false);
        m_isJammingRunning = false;
    }
}

// =================================================================
// 手动模式业务逻辑
// =================================================================

void DeviceManager::setSystemMode(SystemMode mode)
{
    if (m_currentMode == mode) return;

    m_currentMode = mode;
    qDebug() << "[DeviceManager] 切换模式 ->" << (mode == SystemMode::Auto ? "自动" : "手动");

    // 切换模式时，为了安全，停止所有当前动作
    stopSpoofing();
}

void DeviceManager::setManualCircular()
{
    if (m_currentMode != SystemMode::Manual) {
        qWarning() << "[DeviceManager] 拒绝操作：当前不是手动模式";
        return;
    }

    qDebug() << "[手动业务] 执行圆周驱离";
    if (m_spoofDriver) {
        m_spoofDriver->setCircularMotion(500.0, 60.0);
    }
}

void DeviceManager::setManualDirection(SpoofDirection dir)
{
    if (m_currentMode != SystemMode::Manual) {
        qWarning() << "[DeviceManager] 拒绝操作：当前不是手动模式";
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

    qDebug() << "[手动业务] 执行定向驱离 -> 方向:" << dirName;

    if (m_spoofDriver) {
        m_spoofDriver->setLinearMotion(15.0, angle);
    }
}

// =================================================================
// 自动模式 / 实时侦测响应逻辑
// =================================================================

void DeviceManager::onRealTimeDroneDetected(const QList<DroneInfo> &drones)
{
    // 只要列表不为空，就视为发现目标
    bool hasDrone = !drones.isEmpty();
    double distance = 0.0;

    if (hasDrone) {
        const DroneInfo &target = drones.first();
        // TODO: 这里将来需要接入 GIS 计算实际距离
        // 目前为了测试逻辑，我们假设如果有 ID，就是进入了 800米 (红区)
        // 你可以根据 target.lat / target.lon 和基站坐标计算真实距离
        distance = 800.0;

        qDebug() << "[实时侦测] 锁定数传目标:" << target.model << "ID:" << target.id;
    }

    // 调用核心决策
    processDecision(hasDrone, distance);
}

void DeviceManager::onRealTimeImageDetected(int count, const QString &desc)
{
    // 如果只有图传(穿越机)，没有数传
    // 这里的 count > 0 也可以作为触发条件
    if (count > 0) {
        // qDebug() << "[实时侦测] 仅图传信号:" << desc;
        // 可选：如果希望穿越机也触发打击，可以解开下面这行，并设定一个虚拟距离
        // processDecision(true, 500.0);
    }
}

void DeviceManager::processDecision(bool hasDrone, double distance)
{
    // 1. 如果不是自动模式，忽略
    if (m_currentMode != SystemMode::Auto) return;

    // 2. 情况A：目标消失
    if (!hasDrone) {
        if (m_isAutoSpoofingRunning || m_isJammingRunning) {
            qDebug() << "[自动决策] 目标消失 -> 全系统停机";
            stopSpoofing(); // 内部会同时停止诱骗和干扰
        }
        return;
    }

    // 3. 情况B：发现目标 -> 启动诱骗
    if (!m_isAutoSpoofingRunning) {
        qDebug() << "[自动决策] 发现威胁 -> 启动诱骗防御";
        startSpoofing(40.0, 116.0); // 启动基础诱骗
        m_isAutoSpoofingRunning = true;
    }

    // 4. 执行战术：始终保持圆周驱离
    if (m_spoofDriver) {
        m_spoofDriver->setCircularMotion(500.0, 60.0);
    }

    // 5. 分级打击决策 (红区判断)
    // 距离 <= 1000米：开启干扰 (Jamming)
    if (distance <= 1000.0) {
        if (!m_isJammingRunning) {
            qDebug() << "[自动决策] !!! 进入红区 (<1000m) !!! -> 开启干扰压制";
            if (m_jammerDriver) {
                m_jammerDriver->setJamming(true);
            }
            m_isJammingRunning = true;
        }
    }
    // 距离 > 1000米：关闭干扰 (只保留诱骗)
    else {
        if (m_isJammingRunning) {
            qDebug() << "[自动决策] 目标离开红区 -> 停止干扰 (保持诱骗)";
            if (m_jammerDriver) {
                m_jammerDriver->setJamming(false);
            }
            m_isJammingRunning = false;
        }
    }
}
