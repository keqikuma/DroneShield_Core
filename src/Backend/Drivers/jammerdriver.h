#ifndef JAMMERDRIVER_H
#define JAMMERDRIVER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include "../DataStructs.h"

class JammerDriver : public QObject
{
    Q_OBJECT
public:
    explicit JammerDriver(QObject *parent = nullptr);
    ~JammerDriver();

    void setTarget(const QString &ip, int port);
    void setJamming(bool enable);
    void setWriteFreq(const QList<JammerConfigData> &configs);
    void setFixedFreq(const QList<JammerConfigData> &configs);

signals:
    // 【新增】这是一个日志信号，专门用来往UI界面发消息
    void sigLog(const QString &message);

private:
    void sendPostRequest(const QString &apiPath, const QJsonObject &json);

    QNetworkAccessManager *m_manager;
    QString m_baseUrl;
};

#endif // JAMMERDRIVER_H
