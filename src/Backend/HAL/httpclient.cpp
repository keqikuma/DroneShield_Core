#include "httpclient.h"
#include <QJsonDocument>
#include <QNetworkRequest>

HttpClient::HttpClient(QObject *parent)
    : QObject(parent)
{
    m_manager = new QNetworkAccessManager(this);
}

void HttpClient::post(const QString &url, const QJsonObject &json)
{
    QNetworkRequest request((QUrl(url)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    QNetworkReply *reply = m_manager->post(request, data);

    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        if (reply->error() == QNetworkReply::NoError) {
            emit requestFinished(true, reply->readAll());
        } else {
            emit requestFinished(false, reply->errorString());
        }
        reply->deleteLater();
    });
}
