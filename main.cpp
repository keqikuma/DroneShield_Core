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
    // 信号连接
    // =======================================================

    // 1. 下行：后端 -> UI (更新日志和目标列表)
    QObject::connect(systemCore, &DeviceManager::sigLogMessage,
                     &w, &MainWindow::slotUpdateLog);
    QObject::connect(systemCore, &DeviceManager::sigTargetsUpdated,
                     &w, &MainWindow::slotUpdateTargets);

    // 2. 上行：UI -> 后端 (控制指令)

    // 2.1 切换 自动/手动 模式
    QObject::connect(&w, &MainWindow::sigSetAutoMode, systemCore, [systemCore](bool enable){
        systemCore->setSystemMode(enable ? SystemMode::Auto : SystemMode::Manual);
    });

    // 2.2 配置手动干扰参数 (频率)
    QObject::connect(&w, &MainWindow::sigConfigJammer,
                     systemCore, &DeviceManager::setJammerConfig);

    // 2.3 执行手动干扰 (开关) -> 对应 Linux 板卡
    QObject::connect(&w, &MainWindow::sigManualJam,
                     systemCore, &DeviceManager::setManualJammer);

    // 2.4 执行手动诱骗 (开关)
    QObject::connect(&w, &MainWindow::sigManualSpoof,
                     systemCore, &DeviceManager::setManualSpoofSwitch);

    // 2.5 执行信号压制 (Relay)
    QObject::connect(&w, &MainWindow::sigControlRelayChannel,
                     systemCore, &DeviceManager::setRelayChannel);
    QObject::connect(&w, &MainWindow::sigControlRelayAll,
                     systemCore, &DeviceManager::setRelayAll);

    // =======================================================

    w.slotUpdateLog("系统核心已加载，正在连接侦测节点...");
    w.slotUpdateLog("等待 Python 脚本数据流...");

    return a.exec();
}
