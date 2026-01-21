/**
 *  udpsender.cpp
 *      Created on: 2026年1月21日
 *          Author: rustinchen
 */

#include "udpsender.h"
#include "../Consts.h" // 确保能读到 Config::IP_LURE_LOGIC
#include <QJsonDocument>
#include <QDebug>

UdpSender::UdpSender(QObject *parent) : QObject(parent)
{
    m_socket = new QUdpSocket(this);
}

void UdpSender::sendCommand(const QString &encode, const QJsonObject &json)
{
    // 1. JSON 转字符串 (Compact模式去空格)
    QJsonDocument doc(json);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    // 2. 计算长度并补零 (例如长度100 -> "0100")
    QString lenStr = QString("%1").arg(jsonData.length(), 4, 10, QChar('0'));

    // 3. 拼接包头: FF + Length + Encode
    QByteArray packet;
    packet.append("FF");
    packet.append(lenStr.toUtf8());
    packet.append(encode.toUtf8());
    packet.append(jsonData);

    // 4. 发送
#ifdef SIMULATION_MODE
    // 模拟模式下发给本地 9099 端口，配合 Python 脚本测试
    m_socket->writeDatagram(packet, QHostAddress::LocalHost, 9099);
    qDebug() << "[模拟 UDP] 发送诱骗包:" << packet;
#else
    m_socket->writeDatagram(packet, QHostAddress(Config::IP_LURE_LOGIC), Config::PORT_LURE_LOGIC);
#endif
}
