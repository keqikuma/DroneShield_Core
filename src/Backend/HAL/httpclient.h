#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <QObject>

class httpclient : public QObject
{
    Q_OBJECT
public:
    explicit httpclient(QObject *parent = nullptr);

signals:
};

#endif // HTTPCLIENT_H
