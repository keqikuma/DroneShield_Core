#include "devicemanager.h"

DeviceManager::DeviceManager(QObject *parent) : QObject(parent)
{
    qDebug() << "[DeviceManager] 系统初始化中...";
    m_spoofDriver = new SpoofDriver(this);
}

DeviceManager::~DeviceManager()
{
}

void DeviceManager::startSpoofing(double lat, double lon, double alt)
{
    qDebug() << "[DeviceManager] 收到指令: 开启诱骗 (Lat:" << lat << " Lon:" << lon << " Alt:" << alt << ")";

    if (m_spoofDriver) {
        // 直接透传给 Driver，参数一一对应，不会错位
        m_spoofDriver->startSpoofing(lat, lon, alt);
    }
}

void DeviceManager::stopSpoofing()
{
    qDebug() << "[DeviceManager] 收到指令: 停止诱骗";
    if (m_spoofDriver) {
        m_spoofDriver->stopSpoofing();
    }
}
