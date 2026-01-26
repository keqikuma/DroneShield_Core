#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QTableWidgetItem>
#include <QDebug>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QGroupBox>

#include "src/UI/radarview.h"
#include "src/UI/jammerconfdialog.h"
#include "src/UI/relaydialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // =============================================
    // 1. 布局比例设置 (修复宽度问题的关键)
    // =============================================

    // 强制三列等宽 (1:1:1)
    ui->gridLayout_Main->setColumnStretch(0, 1);
    ui->gridLayout_Main->setColumnStretch(1, 1);
    ui->gridLayout_Main->setColumnStretch(2, 1);

    // 行比例
    ui->gridLayout_Main->setRowStretch(0, 2);
    ui->gridLayout_Main->setRowStretch(1, 3);

    ui->groupBox_Radar->setMaximumHeight(450);

    // =============================================
    // 2. 初始化雷达控件
    // =============================================
    m_radar = new RadarView(this);
    ui->groupBox_Radar->layout()->addWidget(m_radar);
    if (ui->widgetRadar) ui->widgetRadar->hide();

    // =============================================
    // 3. 重构右侧布局
    // =============================================

    // A. 隐藏旧控件
    ui->btnAutoMode->hide();
    ui->btnSpoof->hide(); // 隐藏旧的诱骗按钮
    if (ui->line) ui->line->hide();

    // B. 创建右侧总容器
    QWidget *rightPanel = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(15);

    // --- Header (系统状态 + 自动模式) ---
    QWidget *headerWidget = new QWidget(this);
    headerWidget->setStyleSheet("background-color: #1E1E1E; border-radius: 8px;");
    headerWidget->setFixedHeight(60);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(15, 0, 15, 0);

    ui->verticalLayout_3->removeWidget(ui->label_SystemStatus);
    ui->label_SystemStatus->setParent(headerWidget);
    headerLayout->addWidget(ui->label_SystemStatus);
    headerLayout->addStretch();

    QLabel *lblAuto = new QLabel("自动接管模式", this);
    lblAuto->setStyleSheet("font-weight: bold; font-size: 14px; color: #E0E0E0;");
    headerLayout->addWidget(lblAuto);
    headerLayout->addSpacing(10);

    m_autoSwitch = new ToggleSwitch(this);
    m_autoSwitch->setFixedSize(50, 28);
    headerLayout->addWidget(m_autoSwitch);

    rightLayout->addWidget(headerWidget);

    // --- Control Box ---
    // 【关键修复】: 移除最大宽度限制，设为 Expanding
    ui->groupBox_Control->setMinimumWidth(0);
    ui->groupBox_Control->setMaximumWidth(16777215); // 解除限制

    QSizePolicy policy = ui->groupBox_Control->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding); // 水平拉伸
    policy.setVerticalPolicy(QSizePolicy::Expanding);   // 垂直拉伸
    ui->groupBox_Control->setSizePolicy(policy);

    rightLayout->addWidget(ui->groupBox_Control, 1);

    // 将 RightPanel 放入主网格 (0,2)
    ui->gridLayout_Main->addWidget(rightPanel, 0, 2, 2, 1);

    // =============================================
    // 4. 新增：垂直列表式诱骗面板
    // =============================================

    QWidget *spoofPanel = new QWidget(this);
    QVBoxLayout *spoofLayout = new QVBoxLayout(spoofPanel);
    spoofLayout->setContentsMargins(0, 5, 0, 15);
    spoofLayout->setSpacing(8); // 行间距

    // (1) 标题
    QLabel *lblTitle = new QLabel("诱骗模式 (SPOOF)", this);
    lblTitle->setStyleSheet("color: #00FF00; font-weight: bold; font-size: 13px;");
    lblTitle->setAlignment(Qt::AlignCenter);
    spoofLayout->addWidget(lblTitle);

    // (2) 分割线
    QFrame *line1 = new QFrame(this);
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    line1->setStyleSheet("color: #444;");
    spoofLayout->addWidget(line1);

    // (3) 创建 5 个垂直排列的开关行
    // 辅助 Lambda：快速创建一行 [文字 ....... 开关]
    auto createRow = [this, spoofLayout](QString text, ToggleSwitch*& ptr) {
        QWidget *rowWidget = new QWidget(this);
        QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(10, 2, 10, 2); // 左右留白

        QLabel *lbl = new QLabel(text, this);
        lbl->setStyleSheet("font-size: 13px; color: #CCC;");

        ptr = new ToggleSwitch(this);
        ptr->setFixedSize(50, 28);

        rowLayout->addWidget(lbl);
        rowLayout->addStretch();
        rowLayout->addWidget(ptr);

        spoofLayout->addWidget(rowWidget);
    };

    createRow("圆周驱离 (默认)", m_tsCircle);
    createRow("定向驱离：东 (E)", m_tsEast);
    createRow("定向驱离：南 (S)", m_tsSouth);
    createRow("定向驱离：西 (W)", m_tsWest);
    createRow("定向驱离：北 (N)", m_tsNorth);

    // 默认选中圆周
    m_tsCircle->setChecked(true);

    // (4) 执行按钮 (Button, 不是 Switch)
    m_btnExecuteSpoof = new QPushButton("开启诱骗 (EXECUTE)", this);
    m_btnExecuteSpoof->setCheckable(true); // 可按下的状态
    m_btnExecuteSpoof->setMinimumHeight(40);
    // 样式：默认灰色，按下变红色/绿色
    m_btnExecuteSpoof->setStyleSheet(
        "QPushButton { background-color: #444; color: white; border-radius: 5px; font-weight: bold; font-size: 14px; margin-top: 10px; }"
        "QPushButton:checked { background-color: #FF4500; color: white; }" // 开启后变红橙色
        );
    spoofLayout->addWidget(m_btnExecuteSpoof);

    // (5) 底部分割线
    QFrame *line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    line2->setStyleSheet("color: #444; margin-top: 5px;");
    spoofLayout->addWidget(line2);

    // 插入到 Control GroupBox 的最顶部
    if (ui->verticalLayout_3) {
        ui->verticalLayout_3->insertWidget(0, spoofPanel);
        spoofPanel->show();
    }

    // =============================================
    // 5. 表格初始化
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

// 辅助：处理 5 个开关的互斥逻辑 (类似单选框)
void MainWindow::handleSpoofToggleMutex(ToggleSwitch* current)
{
    // 如果当前是被关闭了，且没有其他开启的，强制保持一个开启（或者允许全关？通常模式选择应该必选一个）
    // 这里我们允许全关，或者强制选中当前。
    // 为了体验好，我们只处理 "开启" 的情况：开启一个，关闭其他所有。

    if (current->isChecked()) {
        if (current != m_tsCircle) m_tsCircle->setChecked(false);
        if (current != m_tsNorth)  m_tsNorth->setChecked(false);
        if (current != m_tsEast)   m_tsEast->setChecked(false);
        if (current != m_tsSouth)  m_tsSouth->setChecked(false);
        if (current != m_tsWest)   m_tsWest->setChecked(false);
    } else {
        // 如果用户手动关闭了当前选中的，是否要默认回圆周？
        // 建议：如果全部都关了，自动选中圆周，防止无模式
        if (!m_tsCircle->isChecked() && !m_tsNorth->isChecked() &&
            !m_tsEast->isChecked() && !m_tsSouth->isChecked() && !m_tsWest->isChecked()) {
            m_tsCircle->setChecked(true);
        }
    }
}

void MainWindow::initConnections()
{
    // === 1. 自动模式开关 ===
    connect(m_autoSwitch, &ToggleSwitch::toggled, this, [this](bool checked){
        emit sigSetAutoMode(checked);
        slotUpdateLog(checked ? ">>> [模式] 切换至自动接管 (AUTO)" : ">>> [模式] 切换至手动操作 (MANUAL)");
    });

    // === 2. 诱骗模式选择逻辑 (互斥) ===
    // 绑定所有开关到互斥处理函数
    connect(m_tsCircle, &ToggleSwitch::toggled, this, [this](){ handleSpoofToggleMutex(m_tsCircle); });
    connect(m_tsNorth,  &ToggleSwitch::toggled, this, [this](){ handleSpoofToggleMutex(m_tsNorth); });
    connect(m_tsEast,   &ToggleSwitch::toggled, this, [this](){ handleSpoofToggleMutex(m_tsEast); });
    connect(m_tsSouth,  &ToggleSwitch::toggled, this, [this](){ handleSpoofToggleMutex(m_tsSouth); });
    connect(m_tsWest,   &ToggleSwitch::toggled, this, [this](){ handleSpoofToggleMutex(m_tsWest); });

    // === 3. 执行诱骗按钮 (EXECUTE) ===
    connect(m_btnExecuteSpoof, &QPushButton::toggled, this, [this](bool checked){
        if (checked) {
            // --- 开始诱骗 ---
            m_btnExecuteSpoof->setText("停止诱骗 (STOP)");

            // 判断当前选中的是哪个模式，发送对应信号
            if (m_tsCircle->isChecked()) {
                emit sigManualSpoofCircle();
                slotUpdateLog(">>> [指令] 启动诱骗 -> 圆周模式");
            }
            else if (m_tsNorth->isChecked()) { emit sigManualSpoofNorth(); slotUpdateLog(">>> [指令] 启动诱骗 -> 北 (N)"); }
            else if (m_tsEast->isChecked())  { emit sigManualSpoofEast();  slotUpdateLog(">>> [指令] 启动诱骗 -> 东 (E)"); }
            else if (m_tsSouth->isChecked()) { emit sigManualSpoofSouth(); slotUpdateLog(">>> [指令] 启动诱骗 -> 南 (S)"); }
            else if (m_tsWest->isChecked())  { emit sigManualSpoofWest();  slotUpdateLog(">>> [指令] 启动诱骗 -> 西 (W)"); }
        }
        else {
            // --- 停止诱骗 ---
            m_btnExecuteSpoof->setText("开启诱骗 (EXECUTE)");
            emit sigManualSpoof(false); // 发送停止总指令
            slotUpdateLog(">>> [指令] 诱骗已停止");
        }
    });

    // === 4. 信号压制 (Relay) ===
    connect(ui->btnRelayControl, &QPushButton::clicked, this, [this](){
        RelayDialog dlg(this);
        connect(&dlg, &RelayDialog::sigControlChannel, this, &MainWindow::sigControlRelayChannel);
        connect(&dlg, &RelayDialog::sigControlAll, this, &MainWindow::sigControlRelayAll);
        dlg.exec();
    });

    // === 5. 干扰配置与执行 ===
    connect(ui->btnJammerConfig, &QPushButton::clicked, this, [this](){
        JammerConfigDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            auto uiConfigs = dlg.getConfigs();
            QList<JammerConfigData> drvConfigs;
            for(auto &c : uiConfigs) {
                JammerConfigData d;
                d.freqType = c.freqType;
                d.startFreq = c.startFreq;
                d.endFreq = c.endFreq;
                drvConfigs.append(d);
            }
            emit sigConfigJammer(drvConfigs);
            slotUpdateLog(">>> 干扰参数已更新");
        }
    });

    connect(ui->btnJammer, &QPushButton::toggled, this, [this](bool checked){
        emit sigManualJam(checked);
        slotUpdateLog(checked ? ">>> [指令] 手动开启干扰" : ">>> [指令] 手动停止干扰");
    });
}

// 槽函数保持不变
void MainWindow::slotUpdateLog(const QString &msg)
{
    QString timeStr = QDateTime::currentDateTime().toString("[HH:mm:ss] ");
    ui->textLog->append(timeStr + msg);
}

void MainWindow::slotUpdateTargets(const QList<DroneInfo> &drones)
{
    // 更新列表
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

    // 更新雷达
    QList<RadarTarget> radarTargets;
    for (const auto &d : drones) {
        RadarTarget t;
        t.id = d.id;
        t.distance = d.distance;
        t.angle = d.azimuth;
        radarTargets.append(t);
    }
    if (m_radar) m_radar->updateTargets(radarTargets);

    // 更新状态
    if (drones.isEmpty()) {
        ui->label_SystemStatus->setText("系统状态: 扫描中...");
        ui->label_SystemStatus->setStyleSheet("color: #00ff00;");
    } else {
        ui->label_SystemStatus->setText(QString("系统状态: 发现威胁 (%1)").arg(drones.size()));
        ui->label_SystemStatus->setStyleSheet("color: #ff0000; font-weight: bold; font-size: 14px;");
    }
}
