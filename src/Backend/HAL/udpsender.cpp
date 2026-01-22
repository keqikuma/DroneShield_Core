/**
 *  udpsender.cpp
 *      Created on: 2026年1月21日
 *          Author: rustinchen
 */

#include "udpsender.h"
#include "../Consts.h"
#include <QJsonDocument>
#include <QNetworkDatagram>
#include <QDebug>

UdpSender::UdpSender(QObject *parent) : QObject(parent)
{
    m_socket = new QUdpSocket(this);
    m_socket->bind(QHostAddress::AnyIPv4, 0);
    connect(m_socket, &QUdpSocket::readyRead, this, &UdpSender::onReadyRead);

    // 初始化为空或默认值
    m_targetIp = Config::DEFAULT_SPOOF_IP;
    m_targetPort = Config::DEFAULT_SPOOF_PORT;
}

// 实现设置目标
void UdpSender::setTarget(const QString &ip, int port)
{
    m_targetIp = ip;
    m_targetPort = port;
    // qDebug() << "[UDP] 目标地址已更新为:" << m_targetIp << ":" << m_targetPort;
}

void UdpSender::sendCommand(const QString &encode, const QJsonObject &json)
{
    QJsonDocument doc(json);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    QString lenStr = QString("%1").arg(jsonData.length(), 4, 10, QChar('0'));

    QByteArray packet;
    packet.append("FF");
    packet.append(lenStr.toUtf8());
    packet.append(encode.toUtf8());
    packet.append(jsonData);

    // 使用成员变量发送，不再使用硬编码常量
    m_socket->writeDatagram(packet, QHostAddress(m_targetIp), m_targetPort);

    if (encode == "602") {
#ifdef SIMULATION_MODE
        qDebug() << "[模拟 UDP] 发送指令:" << packet;
#endif
    }
}

// onReadyRead 保持不变... (请确保包含 statusDataReceived 的发射逻辑)
void UdpSender::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        QByteArray data = datagram.data();

        QByteArray jsonBytes;
        QString code = "";

        if (data.startsWith("FF") && data.length() > 9) {
            code = data.mid(6, 3);
            jsonBytes = data.mid(9);
        } else {
            jsonBytes = data;
        }

        QJsonDocument doc = QJsonDocument::fromJson(jsonBytes);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (code == "600" || obj.contains("iSysSta")) {
                bool isWorking = (obj["iSysSta"].toInt() >= 3);
                emit heartbeatReceived(true, isWorking);
                emit statusDataReceived(obj);
            }
        }
    }
}
