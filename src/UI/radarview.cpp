#include "radarview.h"
#include <QPainter>
#include <QPaintEvent>
#include <QtMath>

RadarView::RadarView(QWidget *parent) : QWidget(parent)
{
    // 设置黑色背景
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("background-color: #000000;");

    m_scanAngle = 0;

    // 启动一个定时器，每 50ms 刷新一次界面，让扫描线转起来
    m_scanTimer = new QTimer(this);
    connect(m_scanTimer, &QTimer::timeout, this, [this](){
        m_scanAngle += 2.0; // 每次转 2 度
        if(m_scanAngle >= 360) m_scanAngle = 0;
        update(); // 触发重绘 (paintEvent)
    });
    m_scanTimer->start(50);
}

void RadarView::updateTargets(const QList<RadarTarget> &targets)
{
    m_targets = targets;
    update(); // 数据变了，立即重画
}

void RadarView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 抗锯齿，让线条平滑

    int w = width();
    int h = height();
    int size = qMin(w, h); // 取宽高的较小值作为直径
    QPoint center(w / 2, h / 2); // 中心点
    int maxRadius = size / 2 - 10; // 半径 (留点边距)

    // ===========================
    // 1. 画雷达网格 (绿色同心圆)
    // ===========================
    painter.setPen(QPen(QColor(0, 255, 0, 100), 1)); // 半透明绿线

    // 画三个圈：500m(红区), 1000m, 1500m(最大量程)
    // 我们假设雷达图边缘代表 1500米
    double maxRange = 1500.0;

    int steps = 3;
    for (int i = 1; i <= steps; ++i) {
        int r = maxRadius * i / steps;
        painter.drawEllipse(center, r, r);
        // 画文字 (500m, 1000m...)
        painter.drawText(center.x() + 5, center.y() - r + 15, QString::number(maxRange * i / steps) + "m");
    }

    // 画十字线
    painter.drawLine(center.x(), center.y() - maxRadius, center.x(), center.y() + maxRadius);
    painter.drawLine(center.x() - maxRadius, center.y(), center.x() + maxRadius, center.y());

    // ===========================
    // 2. 画扫描线 (扇形渐变)
    // ===========================
    QConicalGradient gradient(center, -m_scanAngle); // 锥形渐变
    gradient.setColorAt(0, QColor(0, 255, 0, 150));  // 头部亮绿
    gradient.setColorAt(0.1, QColor(0, 255, 0, 0));  // 尾部透明
    gradient.setColorAt(1, QColor(0, 255, 0, 0));

    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawPie(center.x() - maxRadius, center.y() - maxRadius,
                    maxRadius * 2, maxRadius * 2,
                    0 * 16, 360 * 16); // 画满整个圆，靠 Gradient 做扫描效果

    // ===========================
    // 3. 画目标 (红点)
    // ===========================
    painter.setBrush(Qt::red);
    painter.setPen(Qt::white);

    for (const auto &target : m_targets) {
        // 坐标映射：把 实际距离 映射到 屏幕像素
        double pixelDist = (target.distance / maxRange) * maxRadius;

        // 限制在圈内，别画出去了
        if (pixelDist > maxRadius) pixelDist = maxRadius;

        // 极坐标转笛卡尔坐标
        // Qt 坐标系：Y轴向下，0度在右边。我们需要调整一下让 0度在正北
        double angleRad = qDegreesToRadians(target.angle - 90);

        int px = center.x() + pixelDist * qCos(angleRad);
        int py = center.y() + pixelDist * qSin(angleRad);

        // 画点
        painter.drawEllipse(QPoint(px, py), 5, 5);

        // 画 ID
        painter.drawText(px + 8, py, target.id);
    }
}
