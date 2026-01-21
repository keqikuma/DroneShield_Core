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
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        QByteArray data = datagram.data();

        // 1. 提取协议内容 (去除 FF...Header)
        QByteArray jsonBytes;
        QString code = "";

        if (data.startsWith("FF") && data.length() > 9) {
            code = data.mid(6, 3); // 获取指令码 (例如 "600")
            jsonBytes = data.mid(9);
        } else {
            // 兼容纯 JSON 模式
            jsonBytes = data;
        }

        // 2. 解析 JSON
        QJsonDocument doc = QJsonDocument::fromJson(jsonBytes);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();

            // 如果是 600 (状态上报)，发出完整数据供上层处理
            // Node.js 中是判断 if (lastThree == "600")
            if (code == "600" || obj.contains("iSysSta")) {
                // 1. 发送旧的心跳信号（保持兼容）
                bool isWorking = (obj["iSysSta"].toInt() >= 3);
                emit heartbeatReceived(true, isWorking);

                // 2. 【新增】发送完整数据给驱动层做逻辑判断 (对应 PDF 3.1)
                emit statusDataReceived(obj);
            }

            // 处理命令反馈 (651, 652 等)
            if (obj.contains("iState")) {
                // 命令执行结果反馈
                // qDebug() << "[UDP] 命令反馈:" << code << "结果:" << obj["iState"].toInt();
            }
        }
    }
}
