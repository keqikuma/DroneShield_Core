#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
// 引入 DroneInfo 定义
#include "src/Backend/Drivers/detectiondriver.h"
// 引入 JammerConfigData 定义
#include "src/Backend/Drivers/jammerdriver.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// 前置声明 RadarView
class RadarView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    // 更新日志
    void slotUpdateLog(const QString &msg);
    // 更新目标表格
    void slotUpdateTargets(const QList<DroneInfo> &drones);

signals:
    // 界面上的按钮被点击了，通知后端
    void sigSetAutoMode(bool enable);
    void sigManualJam(bool enable);
    void sigManualSpoof(bool enable);

    // 手动干扰参数配置信号
    void sigConfigJammer(const QList<JammerConfigData> &configs);

    // 继电器压制控制信号 (修复报错的关键)
    void sigControlRelayChannel(int channel, bool on);
    void sigControlRelayAll(bool on);

private:
    Ui::MainWindow *ui;

    // 这里使用了 RadarView 指针，必须要有上面的 class RadarView; 前置声明
    RadarView *m_radar;

    void initConnections();
};
#endif // MAINWINDOW_H
