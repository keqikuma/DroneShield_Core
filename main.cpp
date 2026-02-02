#include "mainwindow.h"
#include <QApplication>
#include <QTimer>
#include <QPushButton>
#include "src/Backend/devicemanager.h"
#include "src/AppStyle.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 设置全局样式
    a.setStyleSheet(getDarkTacticalStyle());

    // 1. 先创建后端核心管理器 (DeviceManager)
    // 此时不指定父对象，因为它需要在 MainWindow 之前存在
    DeviceManager *systemCore = new DeviceManager();

    // 2. 创建主窗口 (MainWindow)，并将核心管理器通过构造函数注入
    MainWindow w(systemCore);

    // 3. 建立生命周期绑定
    // 将 systemCore 的父对象设为 w，这样当窗口 w 销毁时，systemCore 也会被自动 delete
    systemCore->setParent(&w);

    // =======================================================
    // 注意：
    // 1. 下行信号 (后端数据->UI) 已在 MainWindow 的 initConnections 中通过 m_deviceManager 连接
    // 2. 上行信号 (UI指令->后端) 建议也迁移至 MainWindow 内部连接，
    //    或者在此处保留必要的全局指令绑定。
    // =======================================================

    // 如果你希望在 main 中保留上行控制信号的显式连接（匹配原代码逻辑）：
    QObject::connect(&w, &MainWindow::sigSetAutoMode, systemCore, [systemCore](bool enable){
        systemCore->setSystemMode(enable ? SystemMode::Auto : SystemMode::Manual);
    });
    QObject::connect(&w, &MainWindow::sigConfigJammer, systemCore, &DeviceManager::setJammerConfig);
    QObject::connect(&w, &MainWindow::sigManualJam, systemCore, &DeviceManager::setManualJammer);
    QObject::connect(&w, &MainWindow::sigControlRelayChannel, systemCore, &DeviceManager::setRelayChannel);
    QObject::connect(&w, &MainWindow::sigControlRelayAll, systemCore, &DeviceManager::setRelayAll);
    QObject::connect(&w, &MainWindow::sigManualSpoof, systemCore, &DeviceManager::setManualSpoofSwitch);
    QObject::connect(&w, &MainWindow::sigManualSpoofCircle, systemCore, &DeviceManager::setManualCircular);
    
    // 定向诱骗方向快捷连接
    QObject::connect(&w, &MainWindow::sigManualSpoofNorth, systemCore, [systemCore](){ systemCore->setManualDirection(SpoofDirection::North); });
    QObject::connect(&w, &MainWindow::sigManualSpoofEast,  systemCore, [systemCore](){ systemCore->setManualDirection(SpoofDirection::East); });
    QObject::connect(&w, &MainWindow::sigManualSpoofSouth, systemCore, [systemCore](){ systemCore->setManualDirection(SpoofDirection::South); });
    QObject::connect(&w, &MainWindow::sigManualSpoofWest,  systemCore, [systemCore](){ systemCore->setManualDirection(SpoofDirection::West); });

    // 显示窗口
    w.show();

    // 初始日志输出
    w.slotUpdateLog("系统核心已加载（单例注入模式）...");
    w.slotUpdateLog("正在初始化底层硬件驱动...");

    return a.exec();
}