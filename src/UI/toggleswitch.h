#ifndef TOGGLESWITCH_H
#define TOGGLESWITCH_H

#include <QWidget>
#include <QPropertyAnimation>
#include <QPainter>

class ToggleSwitch : public QWidget
{
    Q_OBJECT
    // 定义一个属性用于动画 (0.0 = 关闭, 1.0 = 开启)
    Q_PROPERTY(double position READ position WRITE setPosition)

public:
    explicit ToggleSwitch(QWidget *parent = nullptr);

    // 获取/设置状态
    bool isChecked() const;
    void setChecked(bool checked);

    // 动画使用的属性访问器
    double position() const { return m_position; }
    void setPosition(double pos);

    // 设置大小建议
    QSize sizeHint() const override;

signals:
    // 状态改变信号
    void toggled(bool checked);

protected:
    // 绘制事件
    void paintEvent(QPaintEvent *event) override;
    // 鼠标点击事件
    void mouseReleaseEvent(QMouseEvent *event) override;
    // 大小改变事件
    void resizeEvent(QResizeEvent *event) override;

private:
    bool m_checked;          // 当前状态
    double m_position;       // 动画位置 (0.0 - 1.0)
    QPropertyAnimation *m_animate; // 动画对象

    // 颜色配置 (iOS/Mac 风格)
    QColor m_onColor = QColor("#34C759");  // 开启颜色 (绿色) 或者 #007AFF (蓝色)
    QColor m_offColor = QColor("#3A3A3C"); // 关闭颜色 (深灰色，适应你的暗黑主题)
    QColor m_knobColor = Qt::white;        // 按钮颜色
};

#endif // TOGGLESWITCH_H
