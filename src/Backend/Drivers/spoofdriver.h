#ifndef SPOOFDRIVER_H
#define SPOOFDRIVER_H

#include <QObject>
#include <QUdpSocket>
#include <QJsonObject>
#include <QJsonDocument>

// 【必须保留】定义驱离方向枚举，否则 CPP 会报错
enum class SpoofDirection {
    North = 0,
    East = 90,
    South = 180,
    West = 270
};

class SpoofDriver : public QObject
{
    Q_OBJECT
public:
    explicit SpoofDriver(const QString &targetIp, int targetPort, QObject *parent = nullptr);
    ~SpoofDriver();

    // 发送指令函数
    void setPosition(double lon, double lat, double alt);
    void setSwitch(bool enable);
    void startCircular(double radius, double cycle);

    // 【修正】参数类型改为 SpoofDirection，与 cpp 保持一致
    void startDirectional(SpoofDirection dir, double speed);

    void sendLogin();

signals:
    void sigSpoofLog(const QString &msg);

    // 解析出基站坐标后，通过此信号发出去
    void sigDevicePosition(double lat, double lng);

private slots:
    void onReadyRead();

private:
    void sendCommand(const QString &code, const QJsonObject &json);
    QString getLocalIP();

    QUdpSocket *m_udpSender;
    QUdpSocket *m_udpReceiver;
    QHostAddress m_targetAddr;
    quint16 m_targetPort;
    const QString SKEY = "123456";
};

#endif // SPOOFDRIVER_H
