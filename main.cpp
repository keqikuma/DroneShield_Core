#include "mainwindow.h"
#include <QApplication>
#include <QTimer>
#include <QPushButton>
#include "src/Backend/devicemanager.h"
#include "src/AppStyle.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyleSheet(getDarkTacticalStyle());

    MainWindow w;
    w.show();

    // 创建后端核心管理器
    DeviceManager *systemCore = new DeviceManager(&w);

    // =======================================================
    // 1. 下行信号：后端 -> UI (更新显示)
    // =======================================================
    QObject::connect(systemCore, &DeviceManager::sigLogMessage,
                     &w, &MainWindow::slotUpdateLog);
    QObject::connect(systemCore, &DeviceManager::sigTargetsUpdated,
                     &w, &MainWindow::slotUpdateTargets);

    // =======================================================
    // 2. 上行信号：UI -> 后端 (控制指令)
    // =======================================================

    // 2.1 自动/手动模式切换
    QObject::connect(&w, &MainWindow::sigSetAutoMode, systemCore, [systemCore](bool enable){
        systemCore->setSystemMode(enable ? SystemMode::Auto : SystemMode::Manual);
    });

    // 2.2 手动干扰 (Linux)
    QObject::connect(&w, &MainWindow::sigConfigJammer,
                     systemCore, &DeviceManager::setJammerConfig);
    QObject::connect(&w, &MainWindow::sigManualJam,
                     systemCore, &DeviceManager::setManualJammer);

    // 2.3 信号压制 (Relay)
    QObject::connect(&w, &MainWindow::sigControlRelayChannel,
                     systemCore, &DeviceManager::setRelayChannel);
    QObject::connect(&w, &MainWindow::sigControlRelayAll,
                     systemCore, &DeviceManager::setRelayAll);

    // 2.4 手动诱骗 (总开关 - 主要用于停止)
    // 当 UI 发出 false 时，调用 setManualSpoofSwitch(false) 关闭射频
    QObject::connect(&w, &MainWindow::sigManualSpoof,
                     systemCore, &DeviceManager::setManualSpoofSwitch);

    // 2.5 【核心新增】手动诱骗模式具体指令
    // 圆周驱离
    QObject::connect(&w, &MainWindow::sigManualSpoofCircle,
                     systemCore, &DeviceManager::setManualCircular);

    // 定向驱离：北
    QObject::connect(&w, &MainWindow::sigManualSpoofNorth, systemCore, [systemCore](){
        systemCore->setManualDirection(SpoofDirection::North);
    });

    // 定向驱离：东
    QObject::connect(&w, &MainWindow::sigManualSpoofEast, systemCore, [systemCore](){
        systemCore->setManualDirection(SpoofDirection::East);
    });

    // 定向驱离：南
    QObject::connect(&w, &MainWindow::sigManualSpoofSouth, systemCore, [systemCore](){
        systemCore->setManualDirection(SpoofDirection::South);
    });

    // 定向驱离：西
    QObject::connect(&w, &MainWindow::sigManualSpoofWest, systemCore, [systemCore](){
        systemCore->setManualDirection(SpoofDirection::West);
    });

    // =======================================================

    w.slotUpdateLog("系统核心已加载，正在连接侦测节点...");
    w.slotUpdateLog("等待 Python 脚本数据流...");

    return a.exec();
}
