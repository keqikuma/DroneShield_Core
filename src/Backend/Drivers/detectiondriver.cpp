#include "detectiondriver.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <utility>
#include "../Consts.h" // 引入常量
#include <QtMath>      // 引入数学库

DetectionDriver::DetectionDriver(QObject *parent) : QObject(parent)
{
    m_socketClient = new SocketIoClient(this);

    // 绑定 SocketIO 事件
    connect(m_socketClient, &SocketIoClient::eventReceived, this,
            [this](const QString &name, const QJsonValue &data){

                if (name == "droneStatus") {
                    handleDroneStatus(data);
                } else if (name == "imageStatus") {
                    handleImageStatus(data);
                }
            });
}

void DetectionDriver::connectToDevice(const QString &ip, int port)
{
    QString url = QString("ws://%1:%2").arg(ip).arg(port);
    m_socketClient->connectToServer(url);
}

void DetectionDriver::handleDroneStatus(const QJsonValue &data)
{
    QList<DroneInfo> droneList;
    if (data.isArray()) {
        const QJsonArray arr = data.toArray();
        for (const auto &val : std::as_const(arr)) {
            QJsonObject obj = val.toObject();
            QJsonObject info = obj["uav_info"].toObject();
            if (info.isEmpty()) info = obj["trace"].toObject();

            DroneInfo d;
            d.id = info["uav_id"].toString();
            d.model = info["model_name"].toString();
            d.lat = info["uav_lat"].toDouble();
            d.lon = info["uav_lng"].toDouble();
            d.freq = info["freq"].toDouble();

            // === 【核心修改】计算真实距离和方位 ===
            // 使用 Consts.h 里定义的基地坐标作为参考点
            d.distance = calculateDistance(Config::BASE_LAT, Config::BASE_LON, d.lat, d.lon);
            d.azimuth  = calculateAzimuth(Config::BASE_LAT, Config::BASE_LON, d.lat, d.lon);
            // ===================================

            droneList.append(d);
        }
    }
    emit droneDetected(droneList);
}

void DetectionDriver::handleImageStatus(const QJsonValue &data)
{
    if (data.isArray()) {
        QJsonArray arr = data.toArray();
        int count = arr.size();
        QString desc = QString("发现 %1 个图传信号").arg(count);
        emit imageDetected(count, desc);
    }
}

// === 数学算法 ===

// 1. Haversine 公式计算距离 (米)
double DetectionDriver::calculateDistance(double lat1, double lon1, double lat2, double lon2)
{
    double dLat = (lat2 - lat1) * Config::DEG_TO_RAD;
    double dLon = (lon2 - lon1) * Config::DEG_TO_RAD;

    double a = sin(dLat / 2) * sin(dLat / 2) +
               cos(lat1 * Config::DEG_TO_RAD) * cos(lat2 * Config::DEG_TO_RAD) *
                   sin(dLon / 2) * sin(dLon / 2);

    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return Config::EARTH_RADIUS * c;
}

// 2. 计算方位角 (0-360度, 正北为0)
double DetectionDriver::calculateAzimuth(double lat1, double lon1, double lat2, double lon2)
{
    double dLon = (lon2 - lon1) * Config::DEG_TO_RAD;
    double y = sin(dLon) * cos(lat2 * Config::DEG_TO_RAD);
    double x = cos(lat1 * Config::DEG_TO_RAD) * sin(lat2 * Config::DEG_TO_RAD) -
               sin(lat1 * Config::DEG_TO_RAD) * cos(lat2 * Config::DEG_TO_RAD) * cos(dLon);

    double brng = atan2(y, x);
    brng = qRadiansToDegrees(brng); // 转为角度
    return fmod(brng + 360.0, 360.0); // 归一化到 0-360
}
