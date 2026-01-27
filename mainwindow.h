#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QButtonGroup>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

// 引入新的统一数据结构
#include "src/Backend/DataStructs.h"

// 引入自定义控件
#include "src/UI/toggleswitch.h"
#include "src/Backend/Drivers/jammerdriver.h" // 只要 JammerConfigData

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

    // === 新的侦测数据槽函数 ===
    void slotUpdateDroneList(const QList<DroneInfo> &drones);
    void slotUpdateImageList(const QList<ImageInfo> &images);
    void slotUpdateAlertCount(int count);
    void slotUpdateDevicePos(double lat, double lng);

signals:
    void sigSetAutoMode(bool enable);
    void sigManualJam(bool enable);
    void sigManualSpoof(bool enable);

    // 具体诱骗模式信号
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

    // === 左侧侦测面板控件 ===
    QLabel *m_lblAlertCount;        // 红色告警数字
    QWidget *m_droneListContainer;  // 无人机列表容器
    QVBoxLayout *m_droneListLayout; // 无人机列表布局
    QWidget *m_imageListContainer;  // 图传列表容器
    QVBoxLayout *m_imageListLayout; // 图传列表布局

    // === 右侧诱骗控制控件 ===
    ToggleSwitch *m_tsCircle;
    ToggleSwitch *m_tsNorth;
    ToggleSwitch *m_tsEast;
    ToggleSwitch *m_tsSouth;
    ToggleSwitch *m_tsWest;
    QPushButton *m_btnExecuteSpoof;

    void initConnections();
    void handleSpoofToggleMutex(ToggleSwitch* current);

    // === 辅助函数：创建卡片 UI ===
    QWidget* createDroneCard(const DroneInfo &info);
    QWidget* createImageCard(const ImageInfo &info);
    QWidget* createInfoRow(const QString &label, const QString &value, bool isHighlight = false);
};

#endif // MAINWINDOW_H
