/**
 *  spoofdriver.cpp
 *      Created on: 2026年1月21日
 *          Author: rustinchen
 */

#include "spoofdriver.h"
#include "../Consts.h"
#include <QDebug>
#include <QDataStream>
#include <QThread>
#include <QCoreApplication>

SpoofDriver::SpoofDriver(QObject *parent) : QObject(parent)
{
    m_rfClient = new TcpClient(this);
    m_udpSender = new UdpSender(this);

    // 监听心跳
    connect(m_udpSender, &UdpSender::heartbeatReceived, this,
            [](bool isOnline, bool isSpoofing){
                static int c = 0;
                if(c++ % 10 == 0) qDebug() << "[SpoofStatus] Device Online:" << isOnline;
            });

    m_rfClient->connectToServer(Config::IP_BOARD_15, Config::PORT_BOARD_15);
}

// ========================================================================
//                              业务流程控制
// ========================================================================

void SpoofDriver::startSpoofing(int type, double lat, double lon)
{
    qDebug() << ">>> [SpoofDriver] 开始诱骗流程 (Type:" << type << ")";

    // 1. 硬件初始化 (TCP) - 严格按照 Node.js bk15bx 顺序
    configureRfBoard(type);

    // 2. 策略下发 (UDP)
    sendStrategy(lat, lon);
}

void SpoofDriver::stopSpoofing()
{
    qDebug() << "<<< [SpoofDriver] 停止诱骗";

    // 1. UDP 关开关
    QJsonObject obj;
    obj["sKey"] = Config::LURE_SKEY;
    obj["iSwitch"] = 0;
    sendUdpCmd("602", obj);

    // 2. TCP 硬件层也可以发指令关闭发射 (参考 tcp.js Set_biubiu_trun(0))
    // Node.js 里是发 EB 90 ... 06 00 ...
    QByteArray payload;
    payload.append((char)0x00);
    sendEb90Cmd(0x06, payload);
}

void SpoofDriver::setLinearSpoofing(double angle)
{
    qDebug() << "[Spoof] 切换为直线驱离, 角度:" << angle;
    // 对应 udp.js setdirction (608)
    QJsonObject obj;
    obj["sKey"] = Config::LURE_SKEY;
    obj["fInitSpeedVal"] = 15.0; // 默认速度
    obj["fInitSpeedHead"] = angle;
    sendUdpCmd("608", obj); // 608: 直线
}

void SpoofDriver::setCircularSpoofing(double radius, double cycle)
{
    qDebug() << "[Spoof] 切换为圆周驱离";
    // 对应 udp.js setmodele(3) -> 610
    QJsonObject obj;
    obj["sKey"] = Config::LURE_SKEY;
    obj["fCirRadius"] = radius;
    obj["fCirCycle"] = cycle;
    obj["iCirRotDir"] = 0;
    sendUdpCmd("610", obj);
}

// ========================================================================
//                              TCP 硬件协议层 (EB 90)
// ========================================================================

void SpoofDriver::configureRfBoard(int type)
{
    // 参考 tcp.js bk15bx(num) 的执行顺序

    // 1. 设置频率 (0x01)
    sendFreq(type);
    QThread::msleep(100);

    // 2. 设置时隙 (0x05)
    sendTimeSlot(1);
    QThread::msleep(100);

    // 3. 设置模组 (0x0C)
    sendModule(1);
    QThread::msleep(100);

    // 4. 设置衰减 (0x02)
    // 根据 type 选择不同的衰减参数
    int atten1, atten2;
    if (type == 0) { // GPS/单独封控 (参考代码逻辑)
        atten1 = Config::ATTEN_29;
        atten2 = Config::ATTEN_30;
    } else { // 配合诱骗
        atten1 = Config::ATTEN_31;
        atten2 = Config::ATTEN_32;
    }
    sendAttenuation(atten1, atten2);
    QThread::msleep(300);

    // 5. 下发波形文件 (0x09 + Data)
    sendWaveFile(type);
}

void SpoofDriver::sendEb90Cmd(quint8 cmdCode, const QByteArray &payload)
{
    // 构造通用 EB 90 协议包
    // 格式: EB 90 00 0A [Code] [Payload...] 5A...55 FF (校验和在最后)

    QByteArray packet;
    QDataStream s(&packet, QIODevice::WriteOnly);
    s.setByteOrder(QDataStream::BigEndian);

    s << (quint8)0xEB << (quint8)0x90; // Header
    s << (quint16)0x000A;              // Length (Node.js固定写了00 0A)
    s << (quint8)cmdCode;              // Command Code

    // 写入 Payload (变长)
    packet.append(payload);

    // 补齐填充位 (Node.js 逻辑：总长度固定，后面补 5A...55)
    // 我们简化处理，按照抓包格式，只要补足到 byte[14] 为 55
    // 这里为了严格匹配 Node.js 的循环校验，我们需要构造一个完整的 16 字节包
    // 这是一个简化实现，模拟 Node.js 的 send_temp 数组填充

    while(packet.size() < 14) {
        packet.append((char)0x5A);
    }
    packet.append((char)0x55); // byte[14]

    // 计算校验和 (byte[15] = sum(0..14))
    quint8 checksum = 0;
    for(int i=0; i<packet.size(); i++) {
        checksum += (quint8)packet[i];
    }
    packet.append((char)checksum); // byte[15]

    // 发送
    // qDebug() << "[TCP-Raw] Send:" << packet.toHex().toUpper();
    m_rfClient->sendCommand(packet);
}

void SpoofDriver::sendFreq(int type)
{
    // Code 0x01
    // GPS=1581 (06 2D), GLONASS=1603 (06 43)
    quint32 freq = (type == 0) ? 1581 : 1603;

    // Node.js 逻辑：转换 hex 字符串再拼 buffer，这里直接位移
    // 实际上 node.js 发的是 5字节 payload: [00][00][00][06][2D] (假设)
    // 严格对照 tcp.js: buf2 是 5 字节
    QByteArray pl;
    pl.append((char)0x00);
    pl.append((char)((freq >> 24) & 0xFF));
    pl.append((char)((freq >> 16) & 0xFF));
    pl.append((char)((freq >> 8) & 0xFF));
    pl.append((char)(freq & 0xFF));

    qDebug() << "[Spoof-TCP] 1.设置频率:" << freq;
    sendEb90Cmd(0x01, pl);
}

void SpoofDriver::sendTimeSlot(int slot)
{
    // Code 0x05
    QByteArray pl;
    pl.append((char)slot);
    qDebug() << "[Spoof-TCP] 2.设置时隙:" << slot;
    sendEb90Cmd(0x05, pl);
}

void SpoofDriver::sendModule(int mode)
{
    // Code 0x0C
    QByteArray pl;
    pl.append((char)mode);
    qDebug() << "[Spoof-TCP] 3.设置模组:" << mode;
    sendEb90Cmd(0x0C, pl);
}

void SpoofDriver::sendAttenuation(int v1, int v2)
{
    // Code 0x02
    // Node.js logic: m_recv_9009db_temp = 195 + 30 / 0.5 = 255 (0xFF)
    quint8 constVal = 0xFF;

    QByteArray pl;
    pl.append((char)v1);
    pl.append((char)v2);
    pl.append((char)constVal);
    pl.append((char)constVal);

    qDebug() << "[Spoof-TCP] 4.设置衰减:" << v1 << v2;
    sendEb90Cmd(0x02, pl);
}

void SpoofDriver::sendWaveFile(int type)
{
    // Code 0x09
    // 1. 发送协议头告诉大小
    // Node.js: size = filesize / 1024 / 8
    // 模拟大小 100
    quint16 size = 100;

    QByteArray pl;
    pl.append((char)((size >> 8) & 0xFF));
    pl.append((char)(size & 0xFF));

    qDebug() << "[Spoof-TCP] 5.发送波形头部, Size:" << size;
    sendEb90Cmd(0x09, pl);

    // 2. 发送实际文件内容
    QThread::msleep(500); // 等待硬件准备
    // 真实场景：读取 Config::WAVE_PATH_GPS
    // 模拟场景：发一段假数据
    QByteArray dummyFile(1024, 0xAA);
    m_rfClient->sendCommand(dummyFile);
}

// ========================================================================
//                              UDP 策略协议层
// ========================================================================

void SpoofDriver::sendStrategy(double lat, double lon)
{
    // 1. 发送坐标 (601)
    QJsonObject pos;
    pos["sKey"] = Config::LURE_SKEY;
    pos["dbLon"] = lon;
    pos["dbLat"] = lat;
    pos["dbAlt"] = 100.0;
    sendUdpCmd("601", pos);

    // 2. 开启开关 (602)
    QJsonObject sw;
    sw["sKey"] = Config::LURE_SKEY;
    sw["iSwitch"] = 1;
    sendUdpCmd("602", sw);
}

void SpoofDriver::sendUdpCmd(const QString &code, const QJsonObject &json)
{
    m_udpSender->sendCommand(code, json);
}
