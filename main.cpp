#include "mainwindow.h"
#include <QApplication>
#include <QTimer>

// 引入大管家
#include "src/Backend/devicemanager.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    // === 业务启动流程 ===

    // 1. 创建核心控制器
    DeviceManager *systemCore = new DeviceManager(&w);

    // 2. 模拟前端操作：3秒后通过 DeviceManager 开启诱骗
    qDebug() << "=== 等待 3 秒，模拟用户点击开启诱骗 ===";

    QTimer::singleShot(3000, systemCore, [systemCore](){
        // 假设用户想把无人机诱骗到 经纬度 (40.0, 116.0)
        systemCore->startSpoofing(40.0, 116.0);
    });

    // 3. 模拟前端操作：8秒后停止诱骗
    QTimer::singleShot(8000, systemCore, [systemCore](){
        systemCore->stopSpoofing();
    });

    return a.exec();
}
