#ifndef SPOOFDRIVER_H
#define SPOOFDRIVER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QUdpSocket>
#include <QHostAddress>

// 定义驱离方向枚举
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
    // 构造函数：需要传入目标IP (192.168.10.203) 和 端口 (9099)
    explicit SpoofDriver(const QString &targetIp, int targetPort, QObject *parent = nullptr);
    ~SpoofDriver();

    // 发送指令的接口
    void setPosition(double lon, double lat, double alt);
    void setSwitch(bool enable);
    void startCircular(double radius, double cycle);
    void startDirectional(SpoofDirection dir, double speed);

    // 手动重发登录包
    void sendLogin();

signals:
    // 【新增】将底层收到的数据作为日志发给 UI
    void sigSpoofLog(const QString &msg);

private slots:
    // 处理接收到的 UDP 数据 (9098端口)
    void onReadyRead();

private:
    // 内部发送函数
    void sendCommand(const QString &code, const QJsonObject &json);

    // 获取本机在 192.168.10.x 网段的 IP
    QString getLocalIP();

    // 通信 Socket
    QUdpSocket *m_udpSender;   // 用于发送指令 (到 9099)
    QUdpSocket *m_udpReceiver; // 用于接收上报 (监听 9098)

    // 目标地址
    QHostAddress m_targetAddr;
    quint16 m_targetPort;

    // 密钥 (请确认硬件文档中的真实 Key)
    const QString SKEY = "123456";
};

#endif // SPOOFDRIVER_H
