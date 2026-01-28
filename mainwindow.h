#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QButtonGroup>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QCheckBox>
#include <QStackedWidget>

// 引入统一数据结构
#include "src/Backend/DataStructs.h"

// 引入自定义控件
#include "src/UI/toggleswitch.h"
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
    QLabel *m_lblAlertCount;

    QStackedWidget *m_leftStack;
    QPushButton *m_btnSwitchDrone;
    QPushButton *m_btnSwitchImage;

    QWidget *m_droneListContainer;
    QVBoxLayout *m_droneListLayout;
    QWidget *m_imageListContainer;
    QVBoxLayout *m_imageListLayout;

    // === 右侧诱骗控制控件 (改为 QCheckBox) ===
    // 之前报错是因为头文件里还是 ToggleSwitch *m_ts...
    QCheckBox *m_chkCircle;
    QCheckBox *m_chkNorth;
    QCheckBox *m_chkEast;
    QCheckBox *m_chkSouth;
    QCheckBox *m_chkWest;

    QPushButton *m_btnExecuteSpoof;

    void initConnections();

    // 【关键修改】函数名和参数类型
    void handleSpoofCheckBoxMutex(QCheckBox* current);

    // === 辅助函数 ===
    QWidget* createDroneCard(const DroneInfo &info);
    QWidget* createImageCard(const ImageInfo &info);
    QWidget* createInfoRow(const QString &label, const QString &value, bool isHighlight = false);
};

#endif // MAINWINDOW_H
