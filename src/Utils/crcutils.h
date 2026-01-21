#ifndef CRCUTILS_H
#define CRCUTILS_H

#include <QObject>

class CrcUtils : public QObject
{
    Q_OBJECT
public:
    explicit CrcUtils(QObject *parent = nullptr);

signals:
};

#endif // CRCUTILS_H
