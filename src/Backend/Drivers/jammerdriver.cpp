#include "jammerdriver.h"
#include <QDebug>
#include <QUrl>
#include <QNetworkReply>

JammerDriver::JammerDriver(QObject *parent) : QObject(parent)
{
    m_manager = new QNetworkAccessManager(this);
}

JammerDriver::~JammerDriver() {}

void JammerDriver::setTarget(const QString &ip, int port)
{
    m_baseUrl = QString("http://%1:%2").arg(ip).arg(port);
    // 直接发给 UI 显示
    emit sigLog(QString("[干扰初始化] 目标: %1").arg(m_baseUrl));
}

void JammerDriver::setJamming(bool enable)
{
    QString apiPath = "/interferenceControl";
    QJsonObject json;
    json["switch"] = enable ? 1 : 0;
    sendPostRequest(apiPath, json);
}

void JammerDriver::setWriteFreq(const QList<JammerConfigData> &configs)
{
    QString apiPath = "/setWriteFreq";
    QJsonArray arr;
    for (const auto &cfg : configs) {
        QJsonObject item;
        item["freqType"] = cfg.freqType;
        item["startFreq"] = cfg.startFreq;
        item["endFreq"] = cfg.endFreq;
        item["isSelect"] = 1;
        item["isErrMsg"] = "";
        arr.append(item);
    }
    QJsonObject json;
    json["writeFreq"] = arr;
    sendPostRequest(apiPath, json);
}

void JammerDriver::setFixedFreq(const QList<JammerConfigData> &configs)
{
    QString apiPath = "/setFixedFreq";
    QJsonArray arr;
    for (const auto &cfg : configs) {
        QJsonObject item;
        item["freqType"] = cfg.freqType;
        item["startFreq"] = cfg.startFreq;
        item["endFreq"] = cfg.endFreq;
        item["isSelect"] = 1;
        item["isErrMsg"] = "";
        arr.append(item);
    }
    QJsonObject json;
    json["constFreq"] = arr;
    sendPostRequest(apiPath, json);
}

// ============================================================================
// 【核心修改】通用发送函数 (带 UI 日志反馈)
// ============================================================================
void JammerDriver::sendPostRequest(const QString &apiPath, const QJsonObject &json)
{
    QUrl url(m_baseUrl + apiPath);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 【注意】如果你之前的截图里 Headers 有 Authorization，这里可能需要加
    // request.setRawHeader("Authorization", "Bearer");

    QJsonDocument doc(json);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    // 1. 发送前：把要发的数据显示在 UI 上
    QString logMsg = QString("[HTTP发送] %1\n数据: %2")
                         .arg(apiPath)
                         .arg(QString::fromUtf8(data));
    emit sigLog(logMsg);

    QNetworkReply *reply = m_manager->post(request, data);

    connect(reply, &QNetworkReply::finished, this, [this, reply, apiPath](){
        // 2. 接收后：把结果显示在 UI 上
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray respData = reply->readAll();
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            QString successMsg = QString("[HTTP成功] Code:%1\n回复: %2")
                                     .arg(statusCode)
                                     .arg(QString::fromUtf8(respData)); // 显示服务器到底回了什么
            emit sigLog(successMsg);
        } else {
            // 打印详细错误码，比如 404 Not Found, 500 Internal Error, Connection Refused
            QString errorMsg = QString("[HTTP失败] 错误: %1\n详情: %2")
                                   .arg(reply->errorString())
                                   .arg(QString::fromUtf8(reply->readAll())); // 即使出错，也要看服务器有没有回错误提示
            emit sigLog(errorMsg);
        }
        reply->deleteLater();
    });
}
