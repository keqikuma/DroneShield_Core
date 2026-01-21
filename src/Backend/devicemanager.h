#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QDebug>

// 引入驱动头文件
#include "Drivers/spoofdriver.h"

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager();

    // ==========================================
    // 对外接口 (供 Main 或 Frontend 调用)
    // ==========================================

    /**
     * @brief 开启诱骗功能
     * @param type 0=GPS, 1=GLONASS
     * @param lat 纬度
     * @param lon 经度
     */
    void startSpoofing(int type, double lat, double lon);

    /**
     * @brief 停止诱骗
     */
    void stopSpoofing();

private:
    // 驱动模块实例
    SpoofDriver *m_spoofDriver;
};

#endif // DEVICEMANAGER_H
