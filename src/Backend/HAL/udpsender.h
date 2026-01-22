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

    void sendCommand(const QString &encode, const QJsonObject &json);

signals:
    void heartbeatReceived(bool isOnline, bool isSpoofing);
    void statusDataReceived(QJsonObject data);

private slots:
    void onReadyRead();

private:
    QUdpSocket *m_socket;
    // 目标地址成员变量
    QString m_targetIp;
    int m_targetPort;
};

#endif // UDPSENDER_H
