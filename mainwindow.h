// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "src/Backend/Drivers/detectiondriver.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    // 【接收】更新日志
    void slotUpdateLog(const QString &msg);
    // 【接收】更新目标表格
    void slotUpdateTargets(const QList<DroneInfo> &drones);

signals:
    // 【发送】界面上的按钮被点击了，通知后端
    void sigSetAutoMode(bool enable);
    void sigManualJam(bool enable);
    void sigManualSpoof(bool enable);

private:
    Ui::MainWindow *ui;
    void initConnections(); // 内部信号初始化
};
#endif // MAINWINDOW_H
