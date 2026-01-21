#ifndef JAMMERDRIVER_H
#define JAMMERDRIVER_H

#include <QObject>

class JammerDriver : public QObject
{
    Q_OBJECT
public:
    explicit JammerDriver(QObject *parent = nullptr);

signals:
};

#endif // JAMMERDRIVER_H
