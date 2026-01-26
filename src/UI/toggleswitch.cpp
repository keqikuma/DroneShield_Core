#include "toggleswitch.h"

ToggleSwitch::ToggleSwitch(QWidget *parent) : QWidget(parent),
    m_checked(false),
    m_position(0.0)
{
    // 启用鼠标追踪 (手型光标)
    setCursor(Qt::PointingHandCursor);

    // 初始化动画
    m_animate = new QPropertyAnimation(this, "position", this);
    m_animate->setDuration(200); // 200ms 动画时长
    m_animate->setEasingCurve(QEasingCurve::InOutQuad); // 平滑曲线
}

QSize ToggleSwitch::sizeHint() const
{
    return QSize(50, 30); // 默认大小
}

bool ToggleSwitch::isChecked() const
{
    return m_checked;
}

void ToggleSwitch::setChecked(bool checked)
{
    if (m_checked == checked) return;

    m_checked = checked;

    // 触发动画
    m_animate->stop();
    m_animate->setStartValue(m_position);
    m_animate->setEndValue(m_checked ? 1.0 : 0.0);
    m_animate->start();

    emit toggled(m_checked);
    update();
}

void ToggleSwitch::setPosition(double pos)
{
    m_position = pos;
    update(); // 触发重绘
}

void ToggleSwitch::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    setChecked(!m_checked); // 切换状态
}

void ToggleSwitch::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    update();
}

void ToggleSwitch::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing); // 抗锯齿

    // === 1. 绘制背景轨道 ===
    QRectF rect = this->rect();
    // 稍微缩小一点，留出边缘
    double margin = 2.0;
    QRectF trackRect = rect.adjusted(margin, margin, -margin, -margin);
    double radius = trackRect.height() / 2.0;

    // 计算当前背景颜色 (根据动画位置插值)
    // 简单的线性插值：关闭颜色 -> 开启颜色
    int r = m_offColor.red()   + (m_onColor.red()   - m_offColor.red())   * m_position;
    int g = m_offColor.green() + (m_onColor.green() - m_offColor.green()) * m_position;
    int b = m_offColor.blue()  + (m_onColor.blue()  - m_offColor.blue())  * m_position;
    QColor currentColor(r, g, b);

    p.setBrush(currentColor);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(trackRect, radius, radius);

    // === 2. 绘制圆形按钮 (Knob) ===
    // 计算 Knob 的 X 坐标
    // startX = 左边 + 半径
    // endX = 右边 - 半径
    // 当前X = startX + (endX - startX) * position
    double knobRadius = radius - 2.0; // 稍微比轨道小一点
    double startX = trackRect.left() + radius; // 圆心起始位
    double endX = trackRect.right() - radius;  // 圆心结束位
    double currentX = startX + (endX - startX) * m_position;

    p.setBrush(m_knobColor);
    // 绘制圆
    p.drawEllipse(QPointF(currentX, trackRect.center().y()), knobRadius, knobRadius);
}
