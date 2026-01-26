#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QTableWidgetItem>
#include <QDebug>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>

// 引入自定义控件
#include "src/UI/radarview.h"
#include "src/UI/jammerconfdialog.h"
#include "src/UI/relaydialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // =============================================
    // 1. 布局调整
    // =============================================

    // 行比例 (垂直方向): 雷达占 40%, 日志占 60%
    ui->gridLayout_Main->setRowStretch(0, 2);
    ui->gridLayout_Main->setRowStretch(1, 3);

    // 列比例 (水平方向): 三列等宽 (1:1:1)
    ui->gridLayout_Main->setColumnStretch(0, 1); // 列表
    ui->gridLayout_Main->setColumnStretch(1, 1); // 雷达
    ui->gridLayout_Main->setColumnStretch(2, 1); // 控制

    // 解锁右侧控制面板的宽度限制
    ui->groupBox_Control->setMinimumWidth(0);
    ui->groupBox_Control->setMaximumWidth(16777215);

    // 限制雷达区域的最大高度
    ui->groupBox_Radar->setMaximumHeight(450);

    // =============================================
    // 2. 初始化雷达控件 (RadarView)
    // =============================================
    m_radar = new RadarView(this);
    ui->groupBox_Radar->layout()->addWidget(m_radar);

    if (ui->widgetRadar) {
        ui->widgetRadar->hide();
    }

    // =============================================
    // 3. 替换自动模式按钮为 ToggleSwitch
    // =============================================

    // A. 隐藏原有的普通按钮
    ui->btnAutoMode->hide(); // 隐藏旧按钮
    if (ui->line) ui->line->hide(); // 隐藏旧分割线

    // B. 创建一个新的右侧总容器 (Right Panel)
    QWidget *rightPanel = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(15); // Header 和下方控制框的间距

    // C. 创建顶部 Header 区域 (包含状态 + 开关)
    QWidget *headerWidget = new QWidget(this);
    headerWidget->setStyleSheet("background-color: #1E1E1E; border-radius: 8px;"); // 给个深色背景
    headerWidget->setFixedHeight(60); // 固定高度
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(15, 0, 15, 0);

    // --- C1. 移动系统状态标签 ---
    // 从原来的 layout 中移除，添加到新 layout
    ui->verticalLayout_3->removeWidget(ui->label_SystemStatus);
    ui->label_SystemStatus->setParent(headerWidget);
    headerLayout->addWidget(ui->label_SystemStatus);

    // --- C2. 中间弹簧 ---
    headerLayout->addStretch();

    // --- C3. 自动模式标签 ---
    QLabel *lblAuto = new QLabel("自动接管模式", this);
    lblAuto->setStyleSheet("font-weight: bold; font-size: 14px; color: #E0E0E0;");
    headerLayout->addWidget(lblAuto);
    headerLayout->addSpacing(10);

    // --- C4. 创建 Mac 风格开关 ---
    m_autoSwitch = new ToggleSwitch(this);
    m_autoSwitch->setFixedSize(50, 28); // 调整大小适合 Header
    headerLayout->addWidget(m_autoSwitch);

    // D. 组装右侧面板
    // 1. 添加 Header
    rightLayout->addWidget(headerWidget);

    // 2. 添加原来的 groupBox_Control
    // (Qt 会自动把它从原来的 gridLayout_Main 挪到这里)
    QSizePolicy policy = ui->groupBox_Control->sizePolicy();
    policy.setVerticalPolicy(QSizePolicy::Expanding);
    ui->groupBox_Control->setSizePolicy(policy);

    rightLayout->addWidget(ui->groupBox_Control, 1);

    // E. 将新的 Right Panel 放入主网格布局的 (0, 2) 位置
    // 参数: widget, row, column, rowSpan, colSpan
    ui->gridLayout_Main->addWidget(rightPanel, 0, 2, 2, 1);


    // =============================================
    // 4. 表格初始化
    // =============================================
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
    // === 1. 自动模式开关 (ToggleSwitch) ===
    connect(m_autoSwitch, &ToggleSwitch::toggled, this, [this](bool checked){
        // 发送信号给后端
        emit sigSetAutoMode(checked);
        // 更新日志
        slotUpdateLog(checked ? ">>> [模式] 切换至自动接管 (AUTO)" : ">>> [模式] 切换至手动操作 (MANUAL)");
    });

    // === 2. 信号压制 (Relay) 按钮 ===
    connect(ui->btnRelayControl, &QPushButton::clicked, this, [this](){
        RelayDialog dlg(this);

        // 转发弹窗信号 -> MainWindow 信号 -> 后端
        connect(&dlg, &RelayDialog::sigControlChannel,
                this, &MainWindow::sigControlRelayChannel);
        connect(&dlg, &RelayDialog::sigControlAll,
                this, &MainWindow::sigControlRelayAll);

        dlg.exec();
    });

    // === 3. 干扰参数配置按钮 ===
    connect(ui->btnJammerConfig, &QPushButton::clicked, this, [this](){
        JammerConfigDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            auto uiConfigs = dlg.getConfigs();

            // 转换数据结构
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

    // === 4. 手动干扰按钮 (执行) ===
    connect(ui->btnJammer, &QPushButton::toggled, this, [this](bool checked){
        emit sigManualJam(checked);
        slotUpdateLog(checked ? ">>> [指令] 手动开启干扰 (Linux)" : ">>> [指令] 手动停止干扰");
    });

    // === 5. 手动诱骗按钮 ===
    connect(ui->btnSpoof, &QPushButton::toggled, this, [this](bool checked){
        emit sigManualSpoof(checked);
        slotUpdateLog(checked ? ">>> 手动开启诱骗指令已下发" : ">>> 手动停止诱骗");
    });
}

// === 槽函数实现 ===

void MainWindow::slotUpdateLog(const QString &msg)
{
    QString timeStr = QDateTime::currentDateTime().toString("[HH:mm:ss] ");
    ui->textLog->append(timeStr + msg);
}

void MainWindow::slotUpdateTargets(const QList<DroneInfo> &drones)
{
    // --- 更新列表 ---
    ui->tblTargets->setRowCount(0);

    for (const auto &drone : drones) {
        int row = ui->tblTargets->rowCount();
        ui->tblTargets->insertRow(row);

        ui->tblTargets->setItem(row, 0, new QTableWidgetItem(drone.id));
        ui->tblTargets->setItem(row, 1, new QTableWidgetItem(drone.model));

        QString distStr = QString::number(drone.distance, 'f', 0) + "m";
        ui->tblTargets->setItem(row, 2, new QTableWidgetItem(distStr));

        QTableWidgetItem *statusItem = new QTableWidgetItem("锁定");
        statusItem->setForeground(Qt::red);
        statusItem->setTextAlignment(Qt::AlignCenter);
        ui->tblTargets->setItem(row, 3, statusItem);
    }

    // --- 更新雷达 ---
    QList<RadarTarget> radarTargets;
    for (const auto &d : drones) {
        RadarTarget t;
        t.id = d.id;
        t.distance = d.distance;
        t.angle = d.azimuth;
        radarTargets.append(t);
    }

    if (m_radar) {
        m_radar->updateTargets(radarTargets);
    }

    // --- 更新系统状态标签 ---
    if (drones.isEmpty()) {
        ui->label_SystemStatus->setText("系统状态: 扫描中...");
        ui->label_SystemStatus->setStyleSheet("color: #00ff00;"); // 绿色
    } else {
        ui->label_SystemStatus->setText(QString("系统状态: 发现威胁 (%1)").arg(drones.size()));
        ui->label_SystemStatus->setStyleSheet("color: #ff0000; font-weight: bold; font-size: 14px;"); // 红色加粗
    }
}
