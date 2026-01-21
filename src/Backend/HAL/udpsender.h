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

private:
    QUdpSocket *m_socket;
};

#endif // UDPSENDER_H
