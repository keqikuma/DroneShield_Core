#ifndef JAMMERDRIVER_H
#define JAMMERDRIVER_H

#include <QObject>
#include "../HAL/httpclient.h"

struct JammerConfigData {
    int freqType;
    int startFreq;
    int endFreq;
};

class JammerDriver : public QObject
{
    Q_OBJECT
public:
    explicit JammerDriver(QObject *parent = nullptr);

    void setTarget(const QString &ip, int port);

    // 支持传入配置列表
    void setWriteFreq(const QList<JammerConfigData> &configs);

    // 开/关干扰
    void setJamming(bool enable);

private:
    HttpClient *m_httpClient;
    QString m_baseUrl;
};

#endif // JAMMERDRIVER_H
