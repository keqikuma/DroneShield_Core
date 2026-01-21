#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QDebug>
#include "Drivers/spoofdriver.h"

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager();

    // ==========================================
    // 对外接口
    // ==========================================
    void startSpoofing(double lat, double lon, double alt = 100.0);

    void stopSpoofing();

private:
    SpoofDriver *m_spoofDriver;
};

#endif // DEVICEMANAGER_H
