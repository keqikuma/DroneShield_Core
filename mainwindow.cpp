#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QTableWidgetItem>
#include <QDebug>

// 【注意】请确认你的 RadarView 头文件路径是否正确
// 如果文件在根目录，请改为 #include "radarview.h"
#include "src/UI/radarview.h"
#include "src/UI/jammerconfdialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // =============================================
    // 1. 布局调整 (C++ 代码控制，比 .ui 更稳健)
    // =============================================

    // 行比例 (垂直方向): 雷达占 40%, 日志占 60%
    ui->gridLayout_Main->setRowStretch(0, 2);
    ui->gridLayout_Main->setRowStretch(1, 3);

    // 列比例 (水平方向): 三列等宽 (1:1:1)
    ui->gridLayout_Main->setColumnStretch(0, 1); // 列表
    ui->gridLayout_Main->setColumnStretch(1, 1); // 雷达
    ui->gridLayout_Main->setColumnStretch(2, 1); // 控制

    // 解锁右侧控制面板的宽度限制 (允许它占满 1/3)
    ui->groupBox_Control->setMinimumWidth(0);
    ui->groupBox_Control->setMaximumWidth(16777215);

    // 限制雷达区域的最大高度，防止挤压日志
    ui->groupBox_Radar->setMaximumHeight(450);

    // =============================================
    // 2. 初始化雷达控件 (RadarView)
    // =============================================
    m_radar = new RadarView(this);

    // 将自定义的雷达控件添加到界面布局中
    // 注意：我们在 .ui 里放了一个 layout (verticalLayout_2)，直接加进去即可
    ui->groupBox_Radar->layout()->addWidget(m_radar);

    // 隐藏 .ui 设计器里那个黑色的占位符 widget
    if (ui->widgetRadar) {
        ui->widgetRadar->hide();
    }

    // =============================================
    // 3. 初始化表格样式
    // =============================================
    // 左侧列表变宽了，我们可以让列宽更舒展
    ui->tblTargets->setColumnWidth(0, 120); // ID
    ui->tblTargets->setColumnWidth(1, 120); // 机型
    ui->tblTargets->setColumnWidth(2, 100); // 距离

    // 初始化信号连接
    initConnections();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initConnections()
{
    // === 按钮事件连接 ===

    // 1. 自动模式按钮
    connect(ui->btnAutoMode, &QPushButton::toggled, this, [this](bool checked){
        if(checked) {
            ui->btnAutoMode->setText("🛡️ 自动模式: ON");
            // 样式变亮 (可选，如果用了 QSS 会自动生效)
        } else {
            ui->btnAutoMode->setText("🛡️ 自动接管模式");
        }
        emit sigSetAutoMode(checked);
        slotUpdateLog(checked ? ">>> 用户切换至 [自动接管] 模式" : ">>> 用户切换至 [手动] 模式");
    });

    // 2. 手动干扰按钮
    connect(ui->btnJammer, &QPushButton::toggled, this, [this](bool checked){
        // 发送信号给后端 (main.cpp 连接到 setManualJammer)
        emit sigManualJam(checked);
        slotUpdateLog(checked ? ">>> [指令] 手动开启干扰 (Linux)" : ">>> [指令] 手动停止干扰");
    });

    connect(ui->btnJammerConfig, &QPushButton::clicked, this, [this](){
        JammerConfigDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            auto uiConfigs = dlg.getConfigs();

            // 转换数据结构: UI Struct -> Driver Struct
            QList<JammerConfigData> drvConfigs;
            for(auto &c : uiConfigs) {
                JammerConfigData d;
                d.freqType = c.freqType;
                d.startFreq = c.startFreq;
                d.endFreq = c.endFreq;
                drvConfigs.append(d);
            }

            emit sigConfigJammer(drvConfigs);
            slotUpdateLog(">>> 干扰参数已更新，等待执行");
        }
    });

    // 3. 手动诱骗按钮
    connect(ui->btnSpoof, &QPushButton::toggled, this, [this](bool checked){
        emit sigManualSpoof(checked);
        slotUpdateLog(checked ? ">>> 手动开启诱骗指令已下发" : ">>> 手动停止诱骗");
    });
}

// === 槽函数实现 ===

void MainWindow::slotUpdateLog(const QString &msg)
{
    // 获取当前时间
    QString timeStr = QDateTime::currentDateTime().toString("[HH:mm:ss] ");
    // 追加到底部文本框
    ui->textLog->append(timeStr + msg);
}

void MainWindow::slotUpdateTargets(const QList<DroneInfo> &drones)
{
    // --- Part 1: 更新左侧列表 ---
    ui->tblTargets->setRowCount(0); // 清空旧数据

    for (const auto &drone : drones) {
        int row = ui->tblTargets->rowCount();
        ui->tblTargets->insertRow(row);

        ui->tblTargets->setItem(row, 0, new QTableWidgetItem(drone.id));
        ui->tblTargets->setItem(row, 1, new QTableWidgetItem(drone.model));

        // 【关键修改 1】读取 lat (模拟器把距离放在了 lat 字段里)
        // 格式化为整数显示，例如 "1985m"
        qDebug() << "UI收到数据 -> ID:" << drone.id << " 距离(Lat):" << drone.lat;

        QString distStr = QString::number(drone.distance, 'f', 0) + "m";
        ui->tblTargets->setItem(row, 2, new QTableWidgetItem(distStr));

        QTableWidgetItem *statusItem = new QTableWidgetItem("⚠️ 锁定");
        statusItem->setForeground(Qt::red);
        statusItem->setTextAlignment(Qt::AlignCenter);
        ui->tblTargets->setItem(row, 3, statusItem);
    }

    // --- Part 2: 更新中间雷达 (核心逻辑) ---
    QList<RadarTarget> radarTargets;
    for (const auto &d : drones) {
        RadarTarget t;
        t.id = d.id;

        // 直接使用真实距离
        t.distance = d.distance;

        // 直接使用真实方位 (不再用 ID 算假角度了)
        t.angle = d.azimuth;

        radarTargets.append(t);
    }

    // 刷新雷达显示
    if (m_radar) {
        m_radar->updateTargets(radarTargets);
    }

    // --- Part 3: 更新右侧系统状态 ---
    if (drones.isEmpty()) {
        ui->label_SystemStatus->setText("系统状态: 扫描中...");
        ui->label_SystemStatus->setStyleSheet("color: #00ff00;"); // 绿色
    } else {
        ui->label_SystemStatus->setText(QString("系统状态: 发现威胁 (%1)").arg(drones.size()));
        ui->label_SystemStatus->setStyleSheet("color: #ff0000; font-weight: bold; font-size: 14px;"); // 红色加粗
    }
}
