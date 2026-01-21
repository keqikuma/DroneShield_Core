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
        qWarning() << "[TCP] ❌ 无法读取文件:" << filePath;
        return;
    }

    QByteArray fileContent = file.readAll();
    qint64 size = fileContent.size();
    // 大小单位转换：参考 tcp.js -> size = filesize / 1024 / 8;
    // 注意：这里需要确认 Node.js 代码里的单位。
    // tcp.js: const size = filesize / 1024 / 8;
    // 如果文件很大，这里要小心。假设我们发的是模拟的小文件。

    // 我们先不在这里发头，改为由 SpoofDriver 拼装好头(sendProtocolHeader)，
    // 然后调用 sendCommand 发送，最后再调用这个 sendRawData 发送文件体。
    // 为了灵活性，我们将 sendFile 拆解。

    // 但为了完全复刻 node.js 的 sendfile 函数的封装性：
    // 我们在 SpoofDriver 里拼协议，这里只管发二进制流。

    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        qDebug() << "[TCP] 开始发送文件流，大小:" << size;
        m_socket->write(fileContent);
        m_socket->flush();
    }
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
