#include "jammerdriver.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

JammerDriver::JammerDriver(QObject *parent) : QObject(parent)
{
    m_httpClient = new HttpClient(this);
}

void JammerDriver::setTarget(const QString &ip, int port)
{
    m_baseUrl = QString("http://%1:%2").arg(ip).arg(port);
}

// 【核心修改】根据传入的列表构造 JSON
void JammerDriver::setWriteFreq(const QList<JammerConfigData> &configs)
{
    if (configs.isEmpty()) return;

    QJsonArray arr;
    for (const auto &cfg : configs) {
        QJsonObject item;
        item["freqType"] = cfg.freqType;
        item["startFreq"] = cfg.startFreq;
        item["endFreq"] = cfg.endFreq;
        item["isSelect"] = 1; // 选中
        arr.append(item);
    }

    QJsonObject payload;
    payload["writeFreq"] = arr;

    QString url = m_baseUrl + "/setWriteFreq";
    qDebug() << "[Jammer] 下发手动写频参数...";
    m_httpClient->post(url, payload);
}

void JammerDriver::setJamming(bool enable)
{
    QJsonObject payload;
    payload["switch"] = enable ? 1 : 0;

    QString url = m_baseUrl + "/interferenceControl";
    qDebug() << "[Jammer] 手动干扰开关:" << (enable ? "ON" : "OFF");
    m_httpClient->post(url, payload);
}
