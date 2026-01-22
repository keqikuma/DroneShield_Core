#include "mainwindow.h"
#include <QApplication>
#include <QTimer>
#include "src/Backend/devicemanager.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    // 1. 创建核心控制器
    // 此时它会在构造函数里读取 config.ini，如果不存在则创建
    DeviceManager *systemCore = new DeviceManager(&w);

    // 2. 模拟操作：3秒后开启诱骗
    qDebug() << "=== [TEST] 等待 3 秒，测试配置读取与发送 ===";
    QTimer::singleShot(3000, systemCore, [systemCore](){
        // 开启诱骗：经纬度 40, 116, 高度 100
        // 这应该触发: 619(登录) -> 601(位置) -> 604/603/608(参数) -> 602(开启)
        systemCore->startSpoofing(40.0, 116.0, 100.0);
    });

    // 3. 模拟操作：6秒后停止
    QTimer::singleShot(6000, systemCore, [systemCore](){
        systemCore->stopSpoofing();
    });

    return a.exec();
}
