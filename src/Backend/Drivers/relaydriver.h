#ifndef RELAYDRIVER_H
#define RELAYDRIVER_H

#include <QObject>

class RelayDriver : public QObject
{
    Q_OBJECT
public:
    explicit RelayDriver(QObject *parent = nullptr);

signals:
};

#endif // RELAYDRIVER_H
