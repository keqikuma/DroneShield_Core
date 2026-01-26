#include "detectiondriver.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

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
    // 解析 Python 里的 droneStatus
    QList<DroneInfo> droneList;
    if (data.isArray()) {
        QJsonArray arr = data.toArray();
        for (const auto &val : arr) {
            QJsonObject obj = val.toObject();
            QJsonObject info = obj["uav_info"].toObject(); // 注意这里的字段要和抓包一致

            // 如果 uav_info 为空，尝试从 trace 读取 (兼容穿越机逻辑)
            if (info.isEmpty()) info = obj["trace"].toObject();

            DroneInfo d;
            d.id = info["uav_id"].toString();
            d.model = info["model_name"].toString();
            d.lat = info["uav_lat"].toDouble();
            d.lon = info["uav_lng"].toDouble();
            d.freq = info["freq"].toDouble();

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
