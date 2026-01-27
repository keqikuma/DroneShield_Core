#include "detectiondriver.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <utility>

DetectionDriver::DetectionDriver(QObject *parent) : QObject(parent)
{
    m_socketClient = new SocketIoClient(this);

    // 绑定 SocketIO 事件，分发给不同的解析函数
    connect(m_socketClient, &SocketIoClient::eventReceived, this,
            [this](const QString &name, const QJsonValue &data){

                if (name == "droneStatus") {
                    handleDroneStatus(data);
                }
                else if (name == "imageStatus") {
                    handleImageStatus(data);
                }
                else if (name == "detect_batch") {
                    handleDetectBatch(data);
                }
                else if (name == "info") {
                    handleInfo(data);
                }
            });
}

void DetectionDriver::connectToDevice(const QString &ip, int port)
{
    // Node.js 后端默认端口可能是 3000 (根据 server.js process.env.PORT || 3000)
    // 请确保 config 中传入正确的端口
    QString url = QString("ws://%1:%2").arg(ip).arg(port);
    m_socketClient->connectToServer(url);
}

// 1. 解析无人机列表 (droneStatus)
void DetectionDriver::handleDroneStatus(const QJsonValue &data)
{
    QList<DroneInfo> droneList;
    if (data.isArray()) {
        const QJsonArray arr = data.toArray();
        for (const auto &val : std::as_const(arr)) {
            QJsonObject rootObj = val.toObject();

            // 数据在 uav_info 字段中，有时也会混有 uav_points, pilot_points
            QJsonObject info = rootObj["uav_info"].toObject();

            // 容错：如果没有 uav_info，尝试直接读 trace (模拟数据中有时结构不同)
            if (info.isEmpty() && rootObj.contains("trace")) {
                info = rootObj["trace"].toObject();
            }
            if (info.isEmpty()) continue;

            DroneInfo d;
            d.uav_id        = info["uav_id"].toString();
            d.model_name    = info["model_name"].toString();
            d.distance      = info["distance"].toDouble();
            d.azimuth       = info["azimuth"].toDouble(); // 后端已计算好方位角
            d.uav_lat       = info["uav_lat"].toDouble();
            d.uav_lng       = info["uav_lng"].toDouble();
            d.height        = info["height"].toDouble();
            d.freq          = info["freq"].toDouble();    // 后端已除以10
            d.velocity      = info["velocity"].toString();
            d.pilot_lat     = info["pilot_lat"].toDouble();
            d.pilot_lng     = info["pilot_lng"].toDouble();
            d.pilot_distance= info["pilot_distance"].toDouble();
            d.whiteList     = info["whiteList"].toBool();
            d.uuid          = info["uuid"].toString();
            d.img           = info["img"].toInt();
            d.type          = info["type"].toString();

            droneList.append(d);
        }
    }
    emit sigDroneListUpdated(droneList);
}

// 2. 解析图传列表 (imageStatus)
void DetectionDriver::handleImageStatus(const QJsonValue &data)
{
    QList<ImageInfo> imageList;
    if (data.isArray()) {
        const QJsonArray arr = data.toArray();
        for (const auto &val : std::as_const(arr)) {
            QJsonObject obj = val.toObject();

            ImageInfo img;
            img.id        = obj["id"].toString();
            img.freq      = obj["freq"].toDouble();
            img.amplitude = obj["amplitude"].toDouble();
            img.type      = obj["type"].toInt();
            // mes 是时间戳，可能是大整数
            img.mes       = obj["mes"].toVariant().toLongLong();
            img.first     = obj["first"].toInt();

            imageList.append(img);
        }
    }
    emit sigImageListUpdated(imageList);
}

// 3. 解析告警数量 (detect_batch) -> [数量]
void DetectionDriver::handleDetectBatch(const QJsonValue &data)
{
    if (data.isArray()) {
        QJsonArray arr = data.toArray();
        if (!arr.isEmpty()) {
            int count = arr[0].toInt();
            emit sigAlertCountUpdated(count);
        }
    }
}

// 4. 解析自身定位 (info) -> { lat: ..., lng: ... }
void DetectionDriver::handleInfo(const QJsonValue &data)
{
    if (data.isObject()) {
        QJsonObject obj = data.toObject();
        double lat = obj["lat"].toDouble();
        double lng = obj["lng"].toDouble();
        emit sigDevicePositionUpdated(lat, lng);
    }
}
