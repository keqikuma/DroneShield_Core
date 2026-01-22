#include "devicemanager.h"
#include "../Utils/configloader.h"

DeviceManager::DeviceManager(QObject *parent) : QObject(parent)
{
    qDebug() << "[DeviceManager] 系统初始化中...";

    // 1. 读取配置文件
    ConfigLoader config;
    QString spoofIp = config.getSpoofIp();
    int spoofPort = config.getSpoofPort();

    // 2. 使用配置初始化驱动
    // 传入读取到的 IP 和 Port
    m_spoofDriver = new SpoofDriver(spoofIp, spoofPort, this);
}

DeviceManager::~DeviceManager()
{
    // 即使这里什么都不做，也必须写出来，否则编译报错
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
