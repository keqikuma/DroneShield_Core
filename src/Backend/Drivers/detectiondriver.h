#ifndef DETECTIONDRIVER_H
#define DETECTIONDRIVER_H

#include <QObject>
#include "../HAL/socketioclient.h"

// 定义一个简易结构体传给 UI
struct DroneInfo {
    QString id;
    QString model;
    double lat;
    double lon;
    double freq;
};

class DetectionDriver : public QObject
{
    Q_OBJECT
public:
    explicit DetectionDriver(QObject *parent = nullptr);
    void connectToDevice(const QString &ip, int port);

signals:
    // 发现无人机 (数传)
    void droneDetected(const QList<DroneInfo> &drones);
    // 发现图传信号
    void imageDetected(int count, const QString &desc);

private:
    SocketIoClient *m_socketClient;
    void handleDroneStatus(const QJsonValue &data);
    void handleImageStatus(const QJsonValue &data);
};

#endif // DETECTIONDRIVER_H
