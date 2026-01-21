/**
 *  udpsender.cpp
 *      Created on: 2026年1月21日
 *          Author: rustinchen
 */

#include "udpsender.h"
#include "../Consts.h" // 必须包含，用于读取 IP 和端口常量
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkDatagram>
#include <QDebug>

UdpSender::UdpSender(QObject *parent) : QObject(parent)
{
    m_socket = new QUdpSocket(this);

    // =================================================================
    // 绑定端口接收数据
    // =================================================================
    // 注意：在模拟模式下，为了避免和 Python 脚本 (它绑定了9099) 冲突，
    // C++ 客户端绑定系统自动分配的随机端口 (port=0)。
    // Python 脚本会根据“谁给它发了消息”，就把回复发还给谁。
    //
    // 在真实生产环境，如果硬件是主动向控制端 9099 发心跳，这里可能需要改为:
    // m_socket->bind(QHostAddress::Any, 9099, QUdpSocket::ShareAddress);
    // =================================================================
    m_socket->bind(QHostAddress::AnyIPv4, 0);

    // 连接接收信号
    connect(m_socket, &QUdpSocket::readyRead, this, &UdpSender::onReadyRead);
}

void UdpSender::sendCommand(const QString &encode, const QJsonObject &json)
{
    // 1. 将 JSON 对象转为紧凑格式的字符串 (去空格)
    QJsonDocument doc(json);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    // 2. 计算长度字符串，补零至4位 (例如长度 120 -> "0120")
    // 对应 Node.js: String(num).padStart(4, '0')
    QString lenStr = QString("%1").arg(jsonData.length(), 4, 10, QChar('0'));

    // 3. 拼接协议包: Header(FF) + Length(4) + Encode(3) + JSON
    QByteArray packet;
    packet.append("FF");               // Header
    packet.append(lenStr.toUtf8());    // Length
    packet.append(encode.toUtf8());    // Encode (e.g. "601", "602")
    packet.append(jsonData);           // Payload

    // 4. 发送数据
#ifdef SIMULATION_MODE
    // 模拟模式：发给本机 mock_hardware.py 监听的端口
    // 注意：这里端口必须与 mock_hardware.py 里的 UDP_PORT 一致 (9099)
    m_socket->writeDatagram(packet, QHostAddress::LocalHost, 9099);

    // 仅打印关键日志，避免刷屏
    if (encode == "602") {
        qDebug() << "[模拟 UDP] 发送开关指令:" << packet;
    }
#else
    // 真实模式：发给配置表里的硬件 IP (通常是 192.168.10.203)
    m_socket->writeDatagram(packet, QHostAddress(Config::IP_LURE_LOGIC), Config::PORT_LURE_LOGIC);
#endif
}

void UdpSender::onReadyRead()
{
    // 循环读取所有挂起的数据报
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        QByteArray data = datagram.data();

        // 容错处理：数据太短直接忽略
        if (data.length() < 2) return;

        // 解析逻辑
        // Node.js 代码参考: JSON.parse(str.substring(9))
        // 这意味着真实硬件回传的数据，前面也有 9 字节的头 (FF + Len + Code)

        QByteArray jsonBytes;

        // 尝试判断是否包含协议头
        if (data.startsWith("FF") && data.length() > 9) {
            // 情况 A: 包含完整协议头，截取后 9 位之后的数据
            jsonBytes = data.mid(9);
        } else {
            // 情况 B: 可能是纯 JSON (Python 模拟器偷懒可能只发了 JSON)
            jsonBytes = data;
        }

        // 解析 JSON
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &error);

        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();

            // 提取关键字段 iState
            // iState: 1 = 开启/工作中, 0 = 停止/空闲
            if (obj.contains("iState")) {
                bool isSpoofing = (obj["iState"].toInt() == 1);

                // 发出信号通知 DeviceManager
                // 参数1: isOnline (收到包就说明在线), 参数2: isSpoofing
                emit heartbeatReceived(true, isSpoofing);

                // qDebug() << "[UDP] 收到状态反馈 -> 在线, 诱骗中:" << isSpoofing;
            }
        } else {
            // qDebug() << "[UDP] 解析JSON失败:" << error.errorString() << "原始数据:" << data;
        }
    }
}
