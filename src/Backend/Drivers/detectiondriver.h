#ifndef DETECTIONDRIVER_H
#define DETECTIONDRIVER_H

#include <QObject>

class detectiondriver : public QObject
{
    Q_OBJECT
public:
    explicit detectiondriver(QObject *parent = nullptr);

signals:
};

#endif // DETECTIONDRIVER_H
