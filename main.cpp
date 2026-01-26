#include "mainwindow.h"
#include <QApplication>
#include <QTimer>
#include <QPushButton> // <--- 【关键修复】加上这一行！
#include "src/Backend/devicemanager.h"
#include "src/AppStyle.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyleSheet(getDarkTacticalStyle());

    MainWindow w;
    w.show();

    DeviceManager *systemCore = new DeviceManager(&w);

    // === 信号连接 ===

    // 1. 下行：后端 -> UI
    QObject::connect(systemCore, &DeviceManager::sigLogMessage,
                     &w, &MainWindow::slotUpdateLog);
    QObject::connect(systemCore, &DeviceManager::sigTargetsUpdated,
                     &w, &MainWindow::slotUpdateTargets);

    // 2. 上行：UI -> 后端
    QObject::connect(&w, &MainWindow::sigSetAutoMode, systemCore, [systemCore](bool enable){
        systemCore->setSystemMode(enable ? SystemMode::Auto : SystemMode::Manual);
    });

    w.slotUpdateLog("系统核心已加载，正在连接侦测节点...");
    w.slotUpdateLog("等待 Python 脚本数据流...");

    // 模拟用户在 3秒后点击“自动模式”按钮
    QTimer::singleShot(3000, &w, [&w](){
        auto btn = w.findChild<QPushButton*>("btnAutoMode");
        if (btn) btn->setChecked(true);
    });

    return a.exec();
}
