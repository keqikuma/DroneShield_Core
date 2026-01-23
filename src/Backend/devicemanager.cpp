#include "devicemanager.h"
#include "../Utils/configloader.h"

DeviceManager::DeviceManager(QObject *parent) : QObject(parent)
{
    qDebug() << "[DeviceManager] 系统初始化中...";

    // 默认手动模式
    m_currentMode = SystemMode::Manual;

    // 初始化自动模式状态锁
    m_isAutoSpoofingRunning = false;

    ConfigLoader config;
    // 使用配置初始化驱动
    m_spoofDriver = new SpoofDriver(config.getSpoofIp(), config.getSpoofPort(), this);
}

DeviceManager::~DeviceManager()
{
    // Qt 对象树会自动回收子对象，此处无需手动 delete
}

void DeviceManager::startSpoofing(double lat, double lon, double alt)
{
    qDebug() << "[DeviceManager] 开启诱骗基础流程";
    if (m_spoofDriver) {
        // 注意：这里只启动基础流程（登录、定点、开开关）
        // 具体的运动模式（圆周/直线）由后续的手动/自动逻辑决定
        m_spoofDriver->startSpoofing(lat, lon, alt);
    }
}

void DeviceManager::stopSpoofing()
{
    qDebug() << "[DeviceManager] 停止诱骗";
    if (m_spoofDriver) m_spoofDriver->stopSpoofing();
    m_isAutoSpoofingRunning = false;    // 重置自动状态
}

// =================================================================
// 手动模式业务逻辑
// =================================================================

void DeviceManager::setSystemMode(SystemMode mode)
{
    if (m_currentMode == mode) return;

    m_currentMode = mode;
    qDebug() << "[DeviceManager] 切换模式 ->" << (mode == SystemMode::Auto ? "自动" : "手动");

    // 切换模式时，为了安全，建议先停止当前诱骗
    // 防止手动模式的指令在自动模式下继续生效，反之亦然
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
        // 战术参数：半径500米，周期60秒
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

    // 方向映射
    switch (dir) {
    case SpoofDirection::North:
        angle = 0.0;
        dirName = "北";
        break;
    case SpoofDirection::East:
        angle = 90.0;
        dirName = "东";
        break;
    case SpoofDirection::South:
        angle = 180.0;
        dirName = "南";
        break;
    case SpoofDirection::West:
        angle = 270.0;
        dirName = "西";
        break;
    }

    qDebug() << "[手动业务] 执行定向驱离 -> 方向:" << dirName << "(角度:" << angle << ")";

    if (m_spoofDriver) {
        // 速度设为 15m/s (经验值，模拟无人机常规飞行速度)
        m_spoofDriver->setLinearMotion(15.0, angle);
    }
}

// =================================================================
// 自动模式核心逻辑
// =================================================================

void DeviceManager::updateDetection(bool hasDrone, double distance)
{
    // 1. 如果不是自动模式，直接忽略侦测数据
    if (m_currentMode != SystemMode::Auto) {
        return;
    }

    // 2. 情况A：无人机消失
    if (!hasDrone) {
        if (m_isAutoSpoofingRunning) {
            qDebug() << "[自动决策] 目标消失 -> 停止诱骗";
            stopSpoofing(); // 内部会重置 m_isAutoSpoofingRunning
        }
        return;
    }

    // 3. 情况B：发现无人机

    // 状态机：如果还没开始诱骗，就启动基础流程
    if (!m_isAutoSpoofingRunning) {
        qDebug() << "[自动决策] 侦测到目标，距离:" << distance << "米 -> 启动自动防御";
        // 启动基础诱骗 (默认位置，后续可改为侦测到的无人机经纬度)
        startSpoofing(40.0, 116.0);
        m_isAutoSpoofingRunning = true;
    } else {
        // 仅打印调试信息，避免刷屏
        // qDebug() << "[自动决策] 目标持续追踪，距离:" << distance << "米";
    }

    // 4. 执行战术决策
    // 任务书要求：无论距离远近，诱骗设备都执行圆周驱离
    if (m_spoofDriver) {
        // 持续刷新圆周模式指令 (半径500m, 周期60s)
        m_spoofDriver->setCircularMotion(500.0, 60.0);
    }

    // 5. 区域判断 (为未来压制设备预留接口)
    if (distance <= 1000.0) {
        qDebug() << "[自动决策] !!! 进入红区 (<1000m) !!! -> 维持圆周 + (TODO: 开启压制)";
        // TODO: m_jammerDriver->startJamming();
    } else {
        qDebug() << "[自动决策] 警戒区域 (>1000m) -> 维持圆周";
        // TODO: m_jammerDriver->stopJamming();
    }
}
