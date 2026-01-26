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

    m_targetIp = Config::DEFAULT_SPOOF_IP;
    m_targetPort = Config::DEFAULT_SPOOF_PORT;
}

void UdpSender::setTarget(const QString &ip, int port)
{
    m_targetIp = ip;
    m_targetPort = port;
}

void UdpSender::sendCommand(const QString &encode, const QJsonObject &json)
{
    // ... (保持原有的 sendCommand 实现不变) ...
    QJsonDocument doc(json);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    QString lenStr = QString("%1").arg(jsonData.length(), 4, 10, QChar('0'));

    QByteArray packet;
    packet.append("FF");
    packet.append(lenStr.toUtf8());
    packet.append(encode.toUtf8());
    packet.append(jsonData);

    sendData(packet); // 复用 sendData
}

// 【新增】实现原始数据发送
void UdpSender::sendData(const QByteArray &data)
{
    if (m_targetIp.isEmpty() || m_targetPort == 0) return;

    m_socket->writeDatagram(data, QHostAddress(m_targetIp), m_targetPort);

    // 调试日志 (可选)
    // qDebug() << "[UDP Send] " << data.toHex().toUpper();
}

void UdpSender::onReadyRead()
{
    // ... (保持原有的 onReadyRead 实现不变) ...
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        QByteArray data = datagram.data();
        // ... 解析逻辑 ...
        // 为了确保编译通过，这里简写，请保留你原来的代码
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
                emit statusDataReceived(obj);
            }
        }
    }
}
