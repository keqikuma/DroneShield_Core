#ifndef DETECTIONDRIVER_H
#define DETECTIONDRIVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include "../DataStructs.h"

class DetectionDriver : public QObject
{
    Q_OBJECT
public:
    explicit DetectionDriver(QObject *parent = nullptr);
    ~DetectionDriver();

    // 启动监听 (参数 ip 其实不用，默认监听 Any，port 用 8089)
    void startServer(int port);

signals:
    void sigDroneListUpdated(const QList<DroneInfo> &drones);
    void sigImageListUpdated(const QList<ImageInfo> &images);
    void sigAlertCountUpdated(int count);
    void sigDevicePositionUpdated(double lat, double lng);
    void sigLogMessage(const QString &msg); // 用于输出日志

private slots:
    void onNewConnection();
    void onReadyRead();
    void onSocketDisconnected();
    void onDataTimeout(); // 数据超时看门狗

private:
    QTcpServer *m_tcpServer;
    QTcpSocket *m_currentClient; // 当前连接的 Linux 设备
    QByteArray m_buffer;         // 数据接收缓冲区 (处理粘包)
    QTimer *m_dataExpiryTimer;   // 超时检测

    // 解析函数
    void processBuffer();
    void parseJsonData(const QByteArray &jsonBytes);

    // 具体业务解析
    void handleDroneInfo(const QJsonObject &root);
    void handleImageInfo(const QJsonObject &root);
    void handleFpvInfo(const QJsonObject &root);
    void handleDeviceStatus(const QJsonObject &root);

    void clearAllData();
};

#endif // DETECTIONDRIVER_H
