#include "mainwindow.h"
#include <QApplication>
#include <QTimer>
#include "src/Backend/devicemanager.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    DeviceManager *systemCore = new DeviceManager(&w);

    qDebug() << "=== [TEST] 开始测试手动模式业务逻辑 ===";

    // 1. 3秒后开启诱骗 (基础启动)
    QTimer::singleShot(3000, systemCore, [systemCore](){
        systemCore->startSpoofing(40.0, 116.0);
    });

    // 2. 5秒后，模拟用户点击“向东驱离”
    QTimer::singleShot(5000, systemCore, [systemCore](){
        // 调用我们刚写的新接口
        systemCore->setManualDirection(SpoofDirection::East);
    });

    // 3. 8秒后，模拟用户点击“圆周驱离”
    QTimer::singleShot(8000, systemCore, [systemCore](){
        systemCore->setManualCircular();
    });

    // 4. 12秒停止
    QTimer::singleShot(12000, systemCore, [systemCore](){
        systemCore->stopSpoofing();
    });

    return a.exec();
}
