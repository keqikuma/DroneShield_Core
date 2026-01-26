#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QButtonGroup>
#include <QPushButton>

#include "src/UI/toggleswitch.h"
#include "src/Backend/Drivers/detectiondriver.h"
#include "src/Backend/Drivers/jammerdriver.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class RadarView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void slotUpdateLog(const QString &msg);
    void slotUpdateTargets(const QList<DroneInfo> &drones);

signals:
    void sigSetAutoMode(bool enable);
    void sigManualJam(bool enable);
    void sigManualSpoof(bool enable); // 总开关信号

    // 具体模式信号
    void sigManualSpoofCircle();
    void sigManualSpoofNorth();
    void sigManualSpoofEast();
    void sigManualSpoofSouth();
    void sigManualSpoofWest();

    void sigConfigJammer(const QList<JammerConfigData> &configs);
    void sigControlRelayChannel(int channel, bool on);
    void sigControlRelayAll(bool on);

private:
    Ui::MainWindow *ui;
    RadarView *m_radar;
    ToggleSwitch *m_autoSwitch;

    // 诱骗控制相关控件
    // 5个模式选择开关
    ToggleSwitch *m_tsCircle;
    ToggleSwitch *m_tsNorth;
    ToggleSwitch *m_tsEast;
    ToggleSwitch *m_tsSouth;
    ToggleSwitch *m_tsWest;

    // 执行按钮
    QPushButton *m_btnExecuteSpoof;

    void initConnections();
    // 辅助函数：处理开关互斥
    void handleSpoofToggleMutex(ToggleSwitch* current);
};

#endif // MAINWINDOW_H
