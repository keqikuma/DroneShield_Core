#include "mainwindow.h"
#include <QApplication>
#include <QTimer>
#include "src/Backend/devicemanager.h"
#include "src/AppStyle.h" // <--- 1. 引入刚才写的样式头文件

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 2. 在这里应用全局样式 (这就是 QSS 生效的地方)
    a.setStyleSheet(getDarkTacticalStyle());

    MainWindow w;
    w.show();

    // ... 下面保持你原来的逻辑 ...
    DeviceManager *systemCore = new DeviceManager(&w);

    qDebug() << "=== [TEST] 系统启动：正在连接 Python 模拟器... ===";

    QTimer::singleShot(3000, systemCore, [systemCore](){
        qDebug() << "=== [TEST] 3秒已到 -> 自动切换为 Auto 模式 ===";
        systemCore->setSystemMode(SystemMode::Auto);
        qDebug() << "=== [TEST] 等待 Python 脚本推送无人机数据... ===";
    });

    return a.exec();
}
