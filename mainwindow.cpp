// mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QTableWidgetItem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // =============================================
    // 手动设置布局比例 (替代 .ui 文件中的配置)
    // =============================================

    // 1. 设置行比例
    ui->gridLayout_Main->setRowStretch(0, 2);
    ui->gridLayout_Main->setRowStretch(1, 3);

    // 2. 设置列比例 (水平方向)
    ui->gridLayout_Main->setColumnStretch(0, 1);
    ui->gridLayout_Main->setColumnStretch(1, 1);
    ui->gridLayout_Main->setColumnStretch(2, 1);

    // 3. 调整控件尺寸限制

    ui->groupBox_Control->setMinimumWidth(0);
    ui->groupBox_Control->setMaximumWidth(16777215);

    // B. 保持雷达的高度限制 (防止太高)
    ui->groupBox_Radar->setMaximumHeight(450);

    // =============================================

    // 初始化表格列宽
    ui->tblTargets->setColumnWidth(0, 120);
    ui->tblTargets->setColumnWidth(1, 120);
    ui->tblTargets->setColumnWidth(2, 100);

    initConnections();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initConnections()
{
    // 绑定界面按钮发出的信号
    // 当点击“自动模式”按钮时
    connect(ui->btnAutoMode, &QPushButton::toggled, this, [this](bool checked){
        if(checked) {
            ui->btnAutoMode->setText("自动模式: ON");
            slotUpdateLog(">>> 用户切换至 [自动接管] 模式");
        } else {
            ui->btnAutoMode->setText("自动模式");
            slotUpdateLog(">>> 用户切换至 [手动] 模式");
        }
        emit sigSetAutoMode(checked);
    });

    // 手动干扰
    connect(ui->btnJammer, &QPushButton::toggled, this, [this](bool checked){
        emit sigManualJam(checked);
        slotUpdateLog(checked ? ">>> 手动开启干扰" : ">>> 手动停止干扰");
    });

    // 手动诱骗
    connect(ui->btnSpoof, &QPushButton::toggled, this, [this](bool checked){
        emit sigManualSpoof(checked);
        slotUpdateLog(checked ? ">>> 手动开启诱骗" : ">>> 手动停止诱骗");
    });
}

// === 槽函数实现 ===

void MainWindow::slotUpdateLog(const QString &msg)
{
    QString timeStr = QDateTime::currentDateTime().toString("[HH:mm:ss] ");
    // 追加到文本框底部
    ui->textLog->append(timeStr + msg);
}

void MainWindow::slotUpdateTargets(const QList<DroneInfo> &drones)
{
    // 1. 清空旧数据
    ui->tblTargets->setRowCount(0);

    // 2. 遍历添加新数据
    for (const auto &drone : drones) {
        int row = ui->tblTargets->rowCount();
        ui->tblTargets->insertRow(row);

        // ID
        ui->tblTargets->setItem(row, 0, new QTableWidgetItem(drone.id));
        // 机型
        ui->tblTargets->setItem(row, 1, new QTableWidgetItem(drone.model));
        // 距离 (这里暂时还是模拟值，或者从 drone.lat/lon 计算)
        ui->tblTargets->setItem(row, 2, new QTableWidgetItem("800m"));

        // 威胁度/状态
        QTableWidgetItem *statusItem = new QTableWidgetItem("锁定");
        statusItem->setForeground(Qt::red);
        statusItem->setTextAlignment(Qt::AlignCenter);
        ui->tblTargets->setItem(row, 3, statusItem);
    }

    // 更新系统状态标签
    if (drones.isEmpty()) {
        ui->label_SystemStatus->setText("系统状态: 扫描中...");
        ui->label_SystemStatus->setStyleSheet("color: #00ff00;"); // 绿
    } else {
        ui->label_SystemStatus->setText(QString("系统状态: 发现威胁 (%1)").arg(drones.size()));
        ui->label_SystemStatus->setStyleSheet("color: #ff0000; font-weight: bold; font-size: 14px;"); // 红
    }
}
