#include "devicemanager.h"

DeviceManager::DeviceManager(QObject *parent) : QObject(parent)
{
    qDebug() << "[DeviceManager] 系统初始化中...";

    // 1. 初始化诱骗驱动
    m_spoofDriver = new SpoofDriver(this);

    // TODO: 这里以后还要初始化 侦测驱动、压制驱动
}

DeviceManager::~DeviceManager()
{
    // Qt 的父子对象机制会自动释放 m_spoofDriver，这里不需要手动 delete
}

void DeviceManager::startSpoofing(int type, double lat, double lon)
{
    qDebug() << "[DeviceManager] 收到指令: 开启诱骗 (Type:" << type << ")";

    // 转发给底层驱动
    if (m_spoofDriver) {
        m_spoofDriver->startSpoofing(type, lat, lon);
    }
}

void DeviceManager::stopSpoofing()
{
    qDebug() << "[DeviceManager] 收到指令: 停止诱骗";

    if (m_spoofDriver) {
        m_spoofDriver->stopSpoofing();
    }
}
