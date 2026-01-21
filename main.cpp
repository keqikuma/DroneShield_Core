#include "mainwindow.h"
#include <QApplication>
#include "src/Backend/Drivers/spoofdriver.h" // 引入驱动
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    // === 测试代码开始 ===
    qDebug() << "=== 测试开始: 3秒后触发诱骗 ===";

    // 创建驱动实例
    SpoofDriver *driver = new SpoofDriver(&w);

    // 延时3秒触发，确保 TCP 连接有时间建立
    QTimer::singleShot(3000, driver, [driver](){
        // 模拟：开启 GPS 诱骗，坐标设为天安门附近
        driver->startSpoofing(0, 39.9087, 116.3975);
    });
    // === 测试代码结束 ===

    return a.exec();
}
