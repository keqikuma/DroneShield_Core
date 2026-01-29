#ifndef DETECTIONDRIVER_H
#define DETECTIONDRIVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QFile>        // 记得加这个
#include <QTextStream>  // 记得加这个
#include <QDateTime>    // 记得加这个
#include "../DataStructs.h"

class DetectionDriver : public QObject
{
    Q_OBJECT
public:
    explicit DetectionDriver(QObject *parent = nullptr);
    ~DetectionDriver();

    void startServer(int port);

signals:
    void sigDroneListUpdated(const QList<DroneInfo> &drones);
    void sigImageListUpdated(const QList<ImageInfo> &images);
    void sigAlertCountUpdated(int count);
    void sigDevicePositionUpdated(double lat, double lng);

    // 【关键修改】 必须补上这行声明，否则编译报错
    void sigLogMessage(const QString &msg);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onSocketDisconnected();
    void onDataTimeout();

private:
    QTcpServer *m_tcpServer;
    QTcpSocket *m_currentClient;
    QByteArray m_buffer;
    QTimer *m_dataExpiryTimer;

    // 解析相关
    void processBuffer();
    void parseJsonData(const QByteArray &jsonBytes);

    void handleDroneInfo(const QJsonObject &root);
    void handleImageInfo(const QJsonObject &root);
    void handleFpvInfo(const QJsonObject &root);
    void handleDeviceStatus(const QJsonObject &root);

    void clearAllData();

    // 日志辅助
    void writeLog(const QString &msg);
};

#endif // DETECTIONDRIVER_H
