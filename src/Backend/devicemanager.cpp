#include "devicemanager.h"
#include "../Utils/configloader.h"

DeviceManager::DeviceManager(QObject *parent) : QObject(parent)
{
    qDebug() << "[DeviceManager] 系统初始化中...";

    // 默认手动模式
    m_currentMode = SystemMode::Manual;

    ConfigLoader config;
    m_spoofDriver = new SpoofDriver(config.getSpoofIp(), config.getSpoofPort(), this);
}

DeviceManager::~DeviceManager() {}

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
}

// =================================================================
// 手动模式业务逻辑
// =================================================================

void DeviceManager::setSystemMode(SystemMode mode)
{
    m_currentMode = mode;
    qDebug() << "[DeviceManager] 切换模式 ->" << (mode == SystemMode::Auto ? "自动" : "手动");
    // TODO: 如果切到自动，可能需要重置一些状态
}

void DeviceManager::setManualCircular()
{
    if (m_currentMode != SystemMode::Manual) {
        qWarning() << "[DeviceManager] 拒绝操作：当前不是手动模式";
        return;
    }

    qDebug() << "[手动业务] 执行圆周驱离";
    if (m_spoofDriver) {
        // 半径500米，周期60秒
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
