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
#include <QMap>
#include <QTimer>
#include <QDateTime>

// 引入统一数据结构
#include "src/Backend/DataStructs.h"

// 引入自定义控件
#include "src/UI/toggleswitch.h"
#include "src/Backend/Drivers/jammerdriver.h"
#include "src/Backend/devicemanager.h"

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

    // 数据接收槽
    void slotUpdateDroneList(const QList<DroneInfo> &drones);
    void slotUpdateImageList(const QList<ImageInfo> &images);

    void slotUpdateAlertCount(int count);
    void slotUpdateDevicePos(double lat, double lng);

    // 定时刷新界面
    void onUiRefreshTimeout();

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
    DeviceManager *m_deviceManager;

    // === 左侧面板控件 ===
    QStackedWidget *m_leftStack;

    // 【修改】三个切换按钮 (按 0x02, 0x06, 0x07 顺序)
    QPushButton *m_btnDrone; // 0x02 无人机
    QPushButton *m_btnFPV;   // 0x06 FPV (原频谱位置)
    QPushButton *m_btnImage; // 0x07 图传 (原图传位置)

    // 【修改】三个列表容器
    QWidget *m_droneContainer;
    QVBoxLayout *m_droneLayout;

    QWidget *m_fpvContainer; // FPV 容器
    QVBoxLayout *m_fpvLayout;

    QWidget *m_imageContainer; // 图传容器
    QVBoxLayout *m_imageLayout;

    // === 【核心】数据缓存池 ===
    QMap<QString, DroneInfo> m_droneCache; // 0x02
    QMap<QString, ImageInfo> m_fpvCache;   // 0x06
    QMap<QString, ImageInfo> m_imageCache; // 0x07 (图传/Spectrum)

    QMap<QString, qint64> m_lastSeenTime;

    QTimer *m_uiTimer;

    // === 右侧诱骗控制控件 ===
    QCheckBox *m_chkCircle;
    QCheckBox *m_chkNorth;
    QCheckBox *m_chkEast;
    QCheckBox *m_chkSouth;
    QCheckBox *m_chkWest;

    QPushButton *m_btnExecuteSpoof;

    void initConnections();
    void setupLeftPanel();

    void handleSpoofCheckBoxMutex(QCheckBox* current);
    void cleanExpiredTargets();

    // === 辅助函数 ===
    QWidget* createDroneCard(const DroneInfo &info);
    QWidget* createImageCard(const ImageInfo &info);
};

#endif // MAINWINDOW_H
