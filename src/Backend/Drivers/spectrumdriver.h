#ifndef SPECTRUMDRIVER_H
#define SPECTRUMDRIVER_H

#include <QObject>

class SpectrumDriver : public QObject
{
    Q_OBJECT
public:
    explicit SpectrumDriver(QObject *parent = nullptr);

signals:
};

#endif // SPECTRUMDRIVER_H
