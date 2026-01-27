#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QGroupBox>
#include <QScrollArea>
#include <QTabWidget>
#include <QCheckBox>

#include "src/UI/radarview.h"
#include "src/UI/jammerconfdialog.h"
#include "src/UI/relaydialog.h"

// ============================================================================
// 辅助函数：创建卡片内部的一行信息
// ============================================================================
QWidget* MainWindow::createInfoRow(const QString &label, const QString &value, bool isHighlight) {
    QWidget *w = new QWidget(this);
    QHBoxLayout *l = new QHBoxLayout(w);
    l->setContentsMargins(0, 2, 0, 2); // 紧凑布局
    l->setSpacing(10);

    QLabel *lblKey = new QLabel(label, w);
    lblKey->setStyleSheet("color: #888; font-size: 12px;");
    lblKey->setFixedWidth(70); // 固定标签宽度，对齐好看

    QLabel *lblVal = new QLabel(value, w);
    lblVal->setWordWrap(true); // 允许换行
    if (isHighlight) {
        lblVal->setStyleSheet("color: #00FF00; font-weight: bold; font-size: 13px;");
    } else {
        lblVal->setStyleSheet("color: #DDD; font-size: 12px;");
    }

    l->addWidget(lblKey);
    l->addWidget(lblVal); // 值占据剩余空间
    return w;
}

// ============================================================================
// 辅助函数：创建无人机信息卡片
// ============================================================================
QWidget* MainWindow::createDroneCard(const DroneInfo &info) {
    QFrame *card = new QFrame(this);
    card->setStyleSheet("background-color: #2D2D2D; border: 1px solid #444; border-radius: 6px; margin-bottom: 6px;");
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(0);

    // 1. 标题栏 (机型 + ID)
    QLabel *title = new QLabel(QString("%1 (%2)").arg(info.model_name).arg(info.uav_id), card);
    title->setStyleSheet("color: #FFA500; font-weight: bold; font-size: 14px; border-bottom: 1px solid #555; padding-bottom: 5px; margin-bottom: 5px;");
    layout->addWidget(title);

    // 2. 详细内容 (根据需求 JSON 字段显示)
    layout->addWidget(createInfoRow("距离:", QString::number(info.distance, 'f', 1) + " m", true));
    layout->addWidget(createInfoRow("方位角:", QString::number(info.azimuth, 'f', 1) + "°"));
    layout->addWidget(createInfoRow("频率:", QString::number(info.freq, 'f', 1) + " MHz"));
    layout->addWidget(createInfoRow("高度:", QString::number(info.height, 'f', 1) + " m"));
    layout->addWidget(createInfoRow("无人机坐标:", QString::number(info.uav_lat, 'f', 6) + ", " + QString::number(info.uav_lng, 'f', 6)));
    layout->addWidget(createInfoRow("飞手坐标:", QString::number(info.pilot_lat, 'f', 6) + ", " + QString::number(info.pilot_lng, 'f', 6)));
    layout->addWidget(createInfoRow("飞手距离:", QString::number(info.pilot_distance, 'f', 1) + " m"));
    layout->addWidget(createInfoRow("速度:", info.velocity));
    layout->addWidget(createInfoRow("白名单:", info.whiteList ? "是" : "否"));
    layout->addWidget(createInfoRow("UUID:", info.uuid));

    return card;
}

// ============================================================================
// 辅助函数：创建图传/频谱信息卡片
// ============================================================================
QWidget* MainWindow::createImageCard(const ImageInfo &info) {
    QFrame *card = new QFrame(this);
    card->setStyleSheet("background-color: #2D2D2D; border: 1px solid #444; border-radius: 6px; margin-bottom: 6px;");
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(0);

    // 标题
    QString typeStr = (info.type == 1) ? "FPV 图传信号" : "UAV 无人机信号";
    QLabel *title = new QLabel(typeStr, card);
    title->setStyleSheet("color: #00BFFF; font-weight: bold; font-size: 14px; border-bottom: 1px solid #555; padding-bottom: 5px; margin-bottom: 5px;");
    layout->addWidget(title);

    // 内容
    layout->addWidget(createInfoRow("ID:", info.id));
    layout->addWidget(createInfoRow("频率:", QString::number(info.freq, 'f', 1) + " MHz", true));
    layout->addWidget(createInfoRow("信号强度:", QString::number(info.amplitude, 'f', 1)));

    // 时间戳转换
    QDateTime time = QDateTime::fromMSecsSinceEpoch(info.mes);
    layout->addWidget(createInfoRow("更新时间:", time.toString("HH:mm:ss")));

    return card;
}

// ============================================================================
// 主窗口构造函数
// ============================================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. 布局比例设置
    ui->gridLayout_Main->setColumnStretch(0, 1);
    ui->gridLayout_Main->setColumnStretch(1, 2);
    ui->gridLayout_Main->setColumnStretch(2, 1);
    ui->gridLayout_Main->setRowStretch(0, 4);
    ui->gridLayout_Main->setRowStretch(1, 1);
    // ui->groupBox_Radar->setMaximumHeight(450);

    // 2. 初始化雷达控件
    m_radar = new RadarView(this);
    ui->groupBox_Radar->layout()->addWidget(m_radar);
    if (ui->widgetRadar) ui->widgetRadar->hide();

    // =============================================
    // 3. 重构左侧布局 (侦测目标 + Tab + 列表)
    // =============================================

    // 3.0 清除 UI 文件中可能存在的旧控件
    if (ui->groupBox_Targets->layout()) {
        QLayoutItem *item;
        while ((item = ui->groupBox_Targets->layout()->takeAt(0)) != nullptr) {
            if(item->widget()) delete item->widget();
            delete item;
        }
        delete ui->groupBox_Targets->layout();
    }

    // 3.1 创建左侧主垂直布局
    QVBoxLayout *leftMainLayout = new QVBoxLayout(ui->groupBox_Targets);
    leftMainLayout->setContentsMargins(5, 10, 5, 5);
    leftMainLayout->setSpacing(5);

    // 3.2 顶部 Header (弹簧 + 文字 + 红点)
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->addStretch();

    QLabel *lblAlertTip = new QLabel("告警数量:", this);
    lblAlertTip->setStyleSheet("color: #AAAAAA; font-size: 12px; font-weight: bold; margin-right: 5px;");
    headerLayout->addWidget(lblAlertTip);

    m_lblAlertCount = new QLabel("0", this);
    m_lblAlertCount->setStyleSheet(
        "QLabel {"
        "  background-color: #FF0000;"
        "  color: white;"
        "  border-radius: 9px;"
        "  padding: 0px 6px;"
        "  font-weight: bold;"
        "  font-size: 12px;"
        "  min-height: 18px;"
        "  min-width: 14px;"
        "  qproperty-alignment: AlignCenter;"
        "}"
        );
    m_lblAlertCount->hide();

    headerLayout->addWidget(m_lblAlertCount);
    headerLayout->setContentsMargins(0, 0, 5, 0);
    leftMainLayout->addLayout(headerLayout);

    // 3.3 Tab 切换页
    QTabWidget *tabWidget = new QTabWidget(this);
    tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #444; top: -1px; background: #1E1E1E; }"
        "QTabBar::tab { background: #2D2D2D; color: #AAA; padding: 8px 20px; border: 1px solid #444; border-bottom: none; }"
        "QTabBar::tab:selected { background: #444; color: white; border-bottom: 1px solid #444; }"
        "QTabBar::tab:!selected { margin-top: 2px; }"
        );

    // --- Tab 1: 无人机列表 ---
    QScrollArea *scrollDrone = new QScrollArea();
    scrollDrone->setWidgetResizable(true);
    scrollDrone->setStyleSheet("background: transparent; border: none;");

    m_droneListContainer = new QWidget();
    m_droneListContainer->setStyleSheet("background: transparent;");
    m_droneListLayout = new QVBoxLayout(m_droneListContainer);
    m_droneListLayout->setAlignment(Qt::AlignTop);
    m_droneListLayout->setContentsMargins(0, 0, 0, 0);
    m_droneListLayout->setSpacing(5);
    scrollDrone->setWidget(m_droneListContainer);

    tabWidget->addTab(scrollDrone, "无人机列表");

    // --- Tab 2: 图传/频谱 ---
    QScrollArea *scrollImage = new QScrollArea();
    scrollImage->setWidgetResizable(true);
    scrollImage->setStyleSheet("background: transparent; border: none;");

    m_imageListContainer = new QWidget();
    m_imageListContainer->setStyleSheet("background: transparent;");
    m_imageListLayout = new QVBoxLayout(m_imageListContainer);
    m_imageListLayout->setAlignment(Qt::AlignTop);
    m_imageListLayout->setContentsMargins(0, 0, 0, 0);
    m_imageListLayout->setSpacing(5);
    scrollImage->setWidget(m_imageListContainer);

    tabWidget->addTab(scrollImage, "图传/频谱");

    leftMainLayout->addWidget(tabWidget);

    // =============================================
    // 4. 重构右侧布局
    // =============================================

    // A. 隐藏旧控件
    ui->btnAutoMode->hide();
    ui->btnSpoof->hide();
    if (ui->line) ui->line->hide();

    // B. 创建右侧总容器
    QWidget *rightPanel = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(15);

    // C. 顶部 Header
    QWidget *headerWidget = new QWidget(this);
    headerWidget->setStyleSheet("background-color: #1E1E1E; border-radius: 8px;");
    headerWidget->setFixedHeight(60);
    QHBoxLayout *headerTopLayout = new QHBoxLayout(headerWidget);
    headerTopLayout->setContentsMargins(15, 0, 15, 0);

    ui->verticalLayout_3->removeWidget(ui->label_SystemStatus);
    ui->label_SystemStatus->setParent(headerWidget);
    headerTopLayout->addWidget(ui->label_SystemStatus);
    headerTopLayout->addStretch();

    QLabel *lblAuto = new QLabel("自动接管模式", this);
    lblAuto->setStyleSheet("font-weight: bold; font-size: 14px; color: #E0E0E0;");
    headerTopLayout->addWidget(lblAuto);
    headerTopLayout->addSpacing(10);

    m_autoSwitch = new ToggleSwitch(this);
    m_autoSwitch->setFixedSize(50, 28);
    headerTopLayout->addWidget(m_autoSwitch);

    rightLayout->addWidget(headerWidget);

    // D. Control Box
    ui->groupBox_Control->setMinimumWidth(0);
    ui->groupBox_Control->setMaximumWidth(16777215);
    QSizePolicy policy = ui->groupBox_Control->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    policy.setVerticalPolicy(QSizePolicy::Expanding);
    ui->groupBox_Control->setSizePolicy(policy);
    rightLayout->addWidget(ui->groupBox_Control, 1);

    // E. 放入主布局
    ui->gridLayout_Main->addWidget(rightPanel, 0, 2, 2, 1);

    // =============================================
    // F. 【修改核心】诱骗面板 (Spoof Panel) - CheckBox 版
    // =============================================
    QWidget *spoofPanel = new QWidget(this);
    QVBoxLayout *spoofLayout = new QVBoxLayout(spoofPanel);
    spoofLayout->setContentsMargins(10, 5, 10, 15);
    spoofLayout->setSpacing(10);

    // F1. 标题
    QLabel *lblSpoofTitle = new QLabel("诱骗模式 (SPOOF)", this);
    lblSpoofTitle->setStyleSheet("color: #00FF00; font-weight: bold; font-size: 13px;");
    lblSpoofTitle->setAlignment(Qt::AlignCenter);
    spoofLayout->addWidget(lblSpoofTitle);

    QFrame *line1 = new QFrame(this);
    line1->setFrameShape(QFrame::HLine);
    line1->setStyleSheet("color: #444;");
    spoofLayout->addWidget(line1);

    // F2. 创建 5 个 QCheckBox
    QString chkStyle = "QCheckBox { color: #CCC; font-size: 14px; spacing: 8px; } QCheckBox::indicator { width: 18px; height: 18px; }";

    m_chkCircle = new QCheckBox("圆周驱离 (默认)", this);
    m_chkCircle->setStyleSheet(chkStyle);
    spoofLayout->addWidget(m_chkCircle);

    m_chkNorth = new QCheckBox("定向驱离：北 (North)", this);
    m_chkNorth->setStyleSheet(chkStyle);
    spoofLayout->addWidget(m_chkNorth);

    m_chkEast = new QCheckBox("定向驱离：东 (East)", this);
    m_chkEast->setStyleSheet(chkStyle);
    spoofLayout->addWidget(m_chkEast);

    m_chkSouth = new QCheckBox("定向驱离：南 (South)", this);
    m_chkSouth->setStyleSheet(chkStyle);
    spoofLayout->addWidget(m_chkSouth);

    m_chkWest = new QCheckBox("定向驱离：西 (West)", this);
    m_chkWest->setStyleSheet(chkStyle);
    spoofLayout->addWidget(m_chkWest);

    // 默认选中圆周
    m_chkCircle->setChecked(true);

    // F3. 执行按钮
    m_btnExecuteSpoof = new QPushButton("开启诱骗 (EXECUTE)", this);
    m_btnExecuteSpoof->setCheckable(true);
    m_btnExecuteSpoof->setMinimumHeight(45);
    m_btnExecuteSpoof->setStyleSheet(
        "QPushButton { background-color: #444; color: white; border-radius: 5px; font-weight: bold; font-size: 14px; margin-top: 15px; }"
        "QPushButton:checked { background-color: #FF4500; color: white; }"
        );
    spoofLayout->addWidget(m_btnExecuteSpoof);

    QFrame *line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet("color: #444; margin-top: 5px;");
    spoofLayout->addWidget(line2);

    // F4. 插入到 Control GroupBox
    if (ui->verticalLayout_3) {
        ui->verticalLayout_3->insertWidget(0, spoofPanel);
        spoofPanel->show();
    }

    initConnections();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ============================================================================
// 槽函数实现
// ============================================================================

void MainWindow::slotUpdateLog(const QString &msg)
{
    QString timeStr = QDateTime::currentDateTime().toString("[HH:mm:ss] ");
    ui->textLog->append(timeStr + msg);
}

void MainWindow::slotUpdateDroneList(const QList<DroneInfo> &drones)
{
    // 1. 清空当前列表
    QLayoutItem *child;
    while ((child = m_droneListLayout->takeAt(0)) != nullptr) {
        if(child->widget()) delete child->widget();
        delete child;
    }

    // 2. 重新添加卡片
    for (const auto &drone : drones) {
        m_droneListLayout->addWidget(createDroneCard(drone));
    }

    // --- 3. 更新地图 ---
    QList<RadarTarget> mapTargets;
    for (const auto &d : drones) {
        RadarTarget t;
        t.id = d.uav_id;
        // 【关键修改】不再传距离，而是传经纬度
        t.lat = d.uav_lat;
        t.lng = d.uav_lng;
        t.angle = d.azimuth;
        mapTargets.append(t);
    }
    if (m_radar) m_radar->updateTargets(mapTargets);

    // 4. 更新状态文字
    if (drones.isEmpty()) {
        ui->label_SystemStatus->setText("系统状态: 扫描中...");
        ui->label_SystemStatus->setStyleSheet("color: #00ff00;");
    } else {
        ui->label_SystemStatus->setText(QString("系统状态: 发现威胁 (%1)").arg(drones.size()));
        ui->label_SystemStatus->setStyleSheet("color: #ff0000; font-weight: bold; font-size: 14px;");
    }
}

void MainWindow::slotUpdateImageList(const QList<ImageInfo> &images)
{
    // 1. 清空
    QLayoutItem *child;
    while ((child = m_imageListLayout->takeAt(0)) != nullptr) {
        if(child->widget()) delete child->widget();
        delete child;
    }

    // 2. 添加
    for (const auto &img : images) {
        m_imageListLayout->addWidget(createImageCard(img));
    }
}

void MainWindow::slotUpdateAlertCount(int count)
{
    m_lblAlertCount->setText(QString::number(count));
    if (count > 0) {
        m_lblAlertCount->show();
    } else {
        m_lblAlertCount->hide();
    }
}

void MainWindow::slotUpdateDevicePos(double lat, double lng)
{
    // 将设备位置传给地图作为中心点
    if (m_radar) {
        // 如果正在拖拽地图，可能不希望强制居中，这里看你需求
        // 简单起见，我们每次收到位置都居中
        m_radar->setCenterPosition(lat, lng);
    }
}

// ============================================================================
// 交互逻辑
// ============================================================================

// 【修改】互斥逻辑：CheckBox 版本
void MainWindow::handleSpoofCheckBoxMutex(QCheckBox* current)
{
    if (current->isChecked()) {
        if (current != m_chkCircle) m_chkCircle->setChecked(false);
        if (current != m_chkNorth)  m_chkNorth->setChecked(false);
        if (current != m_chkEast)   m_chkEast->setChecked(false);
        if (current != m_chkSouth)  m_chkSouth->setChecked(false);
        if (current != m_chkWest)   m_chkWest->setChecked(false);
    } else {
        // 如果当前被取消勾选，且没有其他被选中（确保至少选一个），则强制选回圆周
        if (!m_chkCircle->isChecked() && !m_chkNorth->isChecked() &&
            !m_chkEast->isChecked() && !m_chkSouth->isChecked() && !m_chkWest->isChecked()) {
            m_chkCircle->setChecked(true);
        }
    }
}

void MainWindow::initConnections()
{
    connect(m_autoSwitch, &ToggleSwitch::toggled, this, [this](bool checked){
        emit sigSetAutoMode(checked);
        slotUpdateLog(checked ? ">>> [模式] 切换至自动接管 (AUTO)" : ">>> [模式] 切换至手动操作 (MANUAL)");
    });

    // 【修改】连接 CheckBox 的 toggled 信号
    connect(m_chkCircle, &QCheckBox::toggled, this, [this](){ handleSpoofCheckBoxMutex(m_chkCircle); });
    connect(m_chkNorth,  &QCheckBox::toggled, this, [this](){ handleSpoofCheckBoxMutex(m_chkNorth); });
    connect(m_chkEast,   &QCheckBox::toggled, this, [this](){ handleSpoofCheckBoxMutex(m_chkEast); });
    connect(m_chkSouth,  &QCheckBox::toggled, this, [this](){ handleSpoofCheckBoxMutex(m_chkSouth); });
    connect(m_chkWest,   &QCheckBox::toggled, this, [this](){ handleSpoofCheckBoxMutex(m_chkWest); });

    // 【修改】执行按钮逻辑：检查 CheckBox 状态
    connect(m_btnExecuteSpoof, &QPushButton::toggled, this, [this](bool checked){
        if (checked) {
            m_btnExecuteSpoof->setText("停止诱骗 (STOP)");
            // 判断哪个被勾选
            if (m_chkCircle->isChecked()) { emit sigManualSpoofCircle(); slotUpdateLog(">>> [指令] 启动诱骗 -> 圆周模式"); }
            else if (m_chkNorth->isChecked()) { emit sigManualSpoofNorth(); slotUpdateLog(">>> [指令] 启动诱骗 -> 北 (N)"); }
            else if (m_chkEast->isChecked())  { emit sigManualSpoofEast();  slotUpdateLog(">>> [指令] 启动诱骗 -> 东 (E)"); }
            else if (m_chkSouth->isChecked()) { emit sigManualSpoofSouth(); slotUpdateLog(">>> [指令] 启动诱骗 -> 南 (S)"); }
            else if (m_chkWest->isChecked())  { emit sigManualSpoofWest();  slotUpdateLog(">>> [指令] 启动诱骗 -> 西 (W)"); }
        } else {
            m_btnExecuteSpoof->setText("开启诱骗 (EXECUTE)");
            emit sigManualSpoof(false);
            slotUpdateLog(">>> [指令] 诱骗已停止");
        }
    });

    connect(ui->btnRelayControl, &QPushButton::clicked, this, [this](){
        RelayDialog dlg(this);
        connect(&dlg, &RelayDialog::sigControlChannel, this, &MainWindow::sigControlRelayChannel);
        connect(&dlg, &RelayDialog::sigControlAll, this, &MainWindow::sigControlRelayAll);
        dlg.exec();
    });

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
