#include "tcpclient.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QThread>
#include <QTimer>

TcpClient::TcpClient(QObject *parent) : QObject(parent)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(m_socket, &QTcpSocket::stateChanged, this, &TcpClient::onStateChanged);
}

void TcpClient::connectToServer(const QString &ip, int port)
{
    m_targetIp = ip;
    m_targetPort = port;

    // 如果已经在连接或已连接，先断开
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }

#ifdef SIMULATION_MODE
    qDebug() << "[模拟 TCP] 假装连接到" << ip << ":" << port;
    // 模拟延时连接成功
    QTimer::singleShot(500, this, [this](){
        emit connected();
    });
#else
    m_socket->connectToHost(ip, port);
#endif
}

void TcpClient::sendCommand(const QByteArray &data)
{
#ifdef SIMULATION_MODE
    qDebug() << "[模拟 TCP] 发送数据(Hex):" << data.toHex().toUpper();
    return;
#endif

    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(data);
        m_socket->flush();
    } else {
        qWarning() << "[TCP] 未连接，无法发送:" << m_targetIp;
    }
}

void TcpClient::sendFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[TCP] 无法读取文件:" << filePath;
        return;
    }

    QByteArray fileData = file.readAll();
    // 这里需要根据协议补充发送文件长度的头指令 (... 0A 09 [Size] ...)
    // 暂时略过协议封装，只做基础发送
    sendCommand(fileData);
}

bool TcpClient::isConnected() const
{
#ifdef SIMULATION_MODE
    return true;
#else
    return m_socket->state() == QAbstractSocket::ConnectedState;
#endif
}

void TcpClient::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    emit dataReceived(data);
}

void TcpClient::onStateChanged(QAbstractSocket::SocketState state)
{
    if (state == QAbstractSocket::ConnectedState) {
        qDebug() << "[TCP] 连接成功:" << m_targetIp;
        emit connected();
    } else if (state == QAbstractSocket::UnconnectedState) {
        qDebug() << "[TCP] 连接断开:" << m_targetIp;
        emit disconnected();
    }
}
