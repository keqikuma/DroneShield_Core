#ifndef SOCKETIOCLIENT_H
#define SOCKETIOCLIENT_H

#include <QObject>

class socketioclient : public QObject
{
    Q_OBJECT
public:
    explicit socketioclient(QObject *parent = nullptr);

signals:
};

#endif // SOCKETIOCLIENT_H
