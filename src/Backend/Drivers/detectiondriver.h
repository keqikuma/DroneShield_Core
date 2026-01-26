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

    // 清洗后的真实距离 (米)
    double distance;
    // 清洗后的真实方位 (度)
    double azimuth;
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

    // 辅助函数：计算两点间距离 (Haversine公式)
    double calculateDistance(double lat1, double lon1, double lat2, double lon2);
    // 辅助函数：计算方位角
    double calculateAzimuth(double lat1, double lon1, double lat2, double lon2);
};

#endif // DETECTIONDRIVER_H
