#ifndef DETECTIONDRIVER_H
#define DETECTIONDRIVER_H

#include <QObject>
#include "../HAL/socketioclient.h"
#include "../DataStructs.h" // 引入统一数据结构

class DetectionDriver : public QObject
{
    Q_OBJECT
public:
    explicit DetectionDriver(QObject *parent = nullptr);
    void connectToDevice(const QString &ip, int port);

signals:
    // 1. 无人机列表更新
    void sigDroneListUpdated(const QList<DroneInfo> &drones);
    // 2. 图传/频谱列表更新
    void sigImageListUpdated(const QList<ImageInfo> &images);
    // 3. 告警数量更新 (detect_batch)
    void sigAlertCountUpdated(int count);
    // 4. 设备自身定位更新 (info)
    void sigDevicePositionUpdated(double lat, double lng);

private:
    SocketIoClient *m_socketClient;

    // 解析处理函数
    void handleDroneStatus(const QJsonValue &data);
    void handleImageStatus(const QJsonValue &data);
    void handleDetectBatch(const QJsonValue &data);
    void handleInfo(const QJsonValue &data);
};

#endif // DETECTIONDRIVER_H
