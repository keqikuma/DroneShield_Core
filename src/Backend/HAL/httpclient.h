#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>

class HttpClient : public QObject
{
    Q_OBJECT
public:
    explicit HttpClient(QObject *parent = nullptr);

    // 通用 POST 接口
    void post(const QString &url, const QJsonObject &json);

signals:
    void requestFinished(bool success, const QString &response);

private:
    QNetworkAccessManager *m_manager;
};

#endif // HTTPCLIENT_H
