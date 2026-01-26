#ifndef RADARVIEW_H
#define RADARVIEW_H

#include <QWidget>
#include <QList>
#include <QTimer>

struct RadarTarget {
    QString id;
    double distance; // 米
    double angle;    // 角度 (0-360)
};

class RadarView : public QWidget
{
    Q_OBJECT
public:
    explicit RadarView(QWidget *parent = nullptr);

    void updateTargets(const QList<RadarTarget> &targets);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<RadarTarget> m_targets;
    double m_scanAngle;
    QTimer *m_scanTimer;
};

#endif // RADARVIEW_H
