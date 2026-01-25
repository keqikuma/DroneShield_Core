#include "jammerdriver.h"
#include <QJsonArray>
#include <QDebug>

JammerDriver::JammerDriver(QObject *parent) : QObject(parent)
{
    m_httpClient = new HttpClient(this);
}

void JammerDriver::setTarget(const QString &ip, int port)
{
    m_baseUrl = QString("http://%1:%2").arg(ip).arg(port);
}

void JammerDriver::setWriteFreq(int startFreq, int endFreq)
{
    // 参考 python set_params 构造 JSON
    QJsonObject item1;
    item1["freqType"] = 1; // 低频板
    item1["startFreq"] = startFreq;
    item1["endFreq"] = endFreq;
    item1["isSelect"] = 1;

    QJsonObject item2;
    item2["freqType"] = 2; // 高频板
    item2["startFreq"] = startFreq;
    item2["endFreq"] = endFreq;
    item2["isSelect"] = 1;

    QJsonArray arr;
    arr.append(item1);
    arr.append(item2);

    QJsonObject payload;
    payload["writeFreq"] = arr;

    QString url = m_baseUrl + "/setWriteFreq";
    qDebug() << "[Jammer] 下发写频参数:" << startFreq << "-" << endFreq;
    m_httpClient->post(url, payload);
}

void JammerDriver::setJamming(bool enable)
{
    // 参考 python start/stop_jamming
    QJsonObject payload;
    payload["switch"] = enable ? 1 : 0;

    QString url = m_baseUrl + "/interferenceControl";
    qDebug() << "[Jammer] 干扰开关:" << (enable ? "ON" : "OFF");
    m_httpClient->post(url, payload);
}
