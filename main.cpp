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

    qDebug() << "=== [TEST] 开始测试：自动模式 ===";

    // 1. 2秒后：切换到自动模式
    QTimer::singleShot(2000, systemCore, [systemCore](){
        systemCore->setSystemMode(SystemMode::Auto);
    });

    // 2. 4秒后：模拟发现目标，距离 1500米 (应该触发圆周)
    QTimer::singleShot(4000, systemCore, [systemCore](){
        systemCore->updateDetection(true, 1500.0);
    });

    // 3. 7秒后：模拟目标逼近，距离 800米 (日志应提示进入红区，继续圆周)
    QTimer::singleShot(7000, systemCore, [systemCore](){
        systemCore->updateDetection(true, 800.0);
    });

    // 4. 10秒后：模拟目标消失 (应该停止诱骗)
    QTimer::singleShot(10000, systemCore, [systemCore](){
        systemCore->updateDetection(false, 0.0);
    });

    return a.exec();
}
