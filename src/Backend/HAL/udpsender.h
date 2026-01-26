/**
 *  udpsender.h
 *      Created on: 2026年1月21日
 *          Author: rustinchen
 */

#ifndef UDPSENDER_H
#define UDPSENDER_H

#include <QObject>
#include <QUdpSocket>
#include <QJsonObject>

class UdpSender : public QObject
{
    Q_OBJECT
public:
    explicit UdpSender(QObject *parent = nullptr);

    // 设置目标地址
    void setTarget(const QString &ip, int port);

    // 发送封装好的 JSON 指令 (自动加 FF 头)
    void sendCommand(const QString &encode, const QJsonObject &json);

    // 【新增】发送原始数据包 (用于 SpoofDriver 自己封装协议的情况)
    void sendData(const QByteArray &data);

signals:
    void heartbeatReceived(bool isOnline, bool isSpoofing);
    void statusDataReceived(QJsonObject data);

private slots:
    void onReadyRead();

private:
    QUdpSocket *m_socket;
    QString m_targetIp;
    int m_targetPort;
};

#endif // UDPSENDER_H
