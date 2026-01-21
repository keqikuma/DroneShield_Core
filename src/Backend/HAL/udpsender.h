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

    // 发送标准诱骗指令包
    void sendCommand(const QString &encode, const QJsonObject &json);

signals:
    // 当收到设备心跳时，发出此信号
    // isOnline: 设备是否在线, isSpoofing: 是否正在诱骗
    void heartbeatReceived(bool isOnline, bool isSpoofing);

private slots:
    void onReadyRead();

private:
    QUdpSocket *m_socket;
};

#endif // UDPSENDER_H
