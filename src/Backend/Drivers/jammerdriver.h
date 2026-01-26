#ifndef JAMMERDRIVER_H
#define JAMMERDRIVER_H

#include <QObject>
#include "../HAL/httpclient.h"

class JammerDriver : public QObject
{
    Q_OBJECT
public:
    explicit JammerDriver(QObject *parent = nullptr);

    // 设置目标 IP (Linux 板子 IP)
    void setTarget(const QString &ip, int port);

    // 1. 写频 (对应 set_params)
    void setWriteFreq(int startFreq, int endFreq);

    // 2. 开/关干扰 (对应 start_jamming / stop_jamming)
    void setJamming(bool enable);

private:
    HttpClient *m_httpClient;
    QString m_baseUrl;
};

#endif // JAMMERDRIVER_H
