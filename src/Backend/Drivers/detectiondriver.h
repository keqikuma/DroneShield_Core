#ifndef DETECTIONDRIVER_H
#define DETECTIONDRIVER_H

#include <QObject>
#include <QList>
#include <QTimer>
#include <QtWebSockets/QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "../DataStructs.h"

class DetectionDriver : public QObject
{
    Q_OBJECT
public:
    explicit DetectionDriver(QObject *parent = nullptr);
    ~DetectionDriver();

    void startWork(const QString &url);
    void stopWork();

signals:
    void sigDroneListUpdated(const QList<DroneInfo> &drones);
    void sigImageListUpdated(const QList<ImageInfo> &images);
    void sigDevicePositionUpdated(double lat, double lng);
    void sigLog(const QString &msg);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onReconnectTimeout();

    // 【新增】心跳发送槽函数
    void onHeartbeatTimeout();

private:
    void parseDroneStatus(const QJsonArray &dataArr);
    void parseImageStatus(const QJsonArray &dataArr);
    void parseDeviceInfo(const QJsonObject &dataObj);

    // 【新增】解析握手包，启动心跳
    void handleHandshake(const QString &payload);

    QWebSocket *m_webSocket;
    QTimer *m_reconnectTimer;

    // 【新增】心跳定时器
    QTimer *m_heartbeatTimer;

    QString m_targetUrl;
};

#endif // DETECTIONDRIVER_H
