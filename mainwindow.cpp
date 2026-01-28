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
// 无人机卡片
// ============================================================================
QWidget* MainWindow::createDroneCard(const DroneInfo &info) {
    // 1. 创建卡片 Frame
    QFrame *card = new QFrame(this);
    card->setFrameShape(QFrame::Box);
    card->setFrameShadow(QFrame::Plain);

    // 样式表：背景暗灰 (#2D2D2D)，文字白色，边框浅灰
    card->setStyleSheet(
        "QFrame {"
        "  background-color: #2D2D2D;"   // 暗色背景
        "  color: #FFFFFF;"              // 白色文字
        "  border: 1px solid #505050;"   // 边框
        "  border-radius: 4px;"
        "}"
        "QLabel {"
        "  border: none;"                // 内部标签不需要边框
        "  background: transparent;"     // 标签背景透明
        "}"
        );

    // 2. 垂直布局
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setSpacing(4);           // 行间距
    layout->setContentsMargins(10, 10, 10, 10); // 内边距

    // 3. 标题 (加粗，稍微大一点)
    QLabel *lblTitle = new QLabel(QString("机型: %1").arg(info.model_name), card);
    QFont titleFont;
    titleFont.setBold(true);
    titleFont.setPointSize(14); // 字号大一点
    lblTitle->setFont(titleFont);
    // 标题可以用个醒目的颜色 (比如橙色) 或者保持白色，这里用浅黄色区分一下
    lblTitle->setStyleSheet("color: #FFD700;");
    layout->addWidget(lblTitle);

    // 4. 详细信息 (一行一个，恢复所有字段)
    // 使用 lambda 简化重复代码
    auto addRow = [&](QString label, QString value) {
        QLabel *lbl = new QLabel(QString("%1: %2").arg(label).arg(value), card);
        lbl->setStyleSheet("font-size: 12px;"); // 普通文字大小
        layout->addWidget(lbl);
    };

    addRow("ID", info.uav_id);
    addRow("距离", QString::number(info.distance, 'f', 1) + " m");
    addRow("方位角", QString::number(info.azimuth, 'f', 1) + "°");
    addRow("频率", QString::number(info.freq, 'f', 1) + " MHz");
    addRow("高度", QString::number(info.height, 'f', 1) + " m");

    // 坐标保留6位小数
    addRow("无人机坐标", QString("%1, %2").arg(info.uav_lat, 0, 'f', 6).arg(info.uav_lng, 0, 'f', 6));
    addRow("飞手坐标", QString("%1, %2").arg(info.pilot_lat, 0, 'f', 6).arg(info.pilot_lng, 0, 'f', 6));

    addRow("飞手距离", QString::number(info.pilot_distance, 'f', 1) + " m");
    addRow("速度", info.velocity);
    addRow("UUID", info.uuid);

    return card;
}

// ============================================================================
// 图传卡片
// ============================================================================
QWidget* MainWindow::createImageCard(const ImageInfo &info) {
    QFrame *card = new QFrame(this);
    card->setFrameShape(QFrame::Box);
    card->setFrameShadow(QFrame::Plain);

    // 样式表：暗色背景，白色文字
    card->setStyleSheet(
        "QFrame {"
        "  background-color: #2D2D2D;"
        "  color: #FFFFFF;"
        "  border: 1px solid #505050;"
        "  border-radius: 4px;"
        "}"
        "QLabel { border: none; background: transparent; }"
        );

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setSpacing(4);
    layout->setContentsMargins(10, 10, 10, 10);

    // 标题
    QString typeStr = (info.type == 1) ? "信号: FPV图传" : "信号: 无人机";
    QLabel *lblTitle = new QLabel(typeStr, card);
    QFont titleFont;
    titleFont.setBold(true);
    titleFont.setPointSize(14);
    lblTitle->setFont(titleFont);
    lblTitle->setStyleSheet("color: #00BFFF;"); // 蓝色标题
    layout->addWidget(lblTitle);

    // 内容
    layout->addWidget(new QLabel(QString("ID: %1").arg(info.id), card));
    layout->addWidget(new QLabel(QString("频率: %1 MHz").arg(info.freq, 0, 'f', 1), card));
    layout->addWidget(new QLabel(QString("强度: %1").arg(info.amplitude, 0, 'f', 1), card));

    // 时间
    QDateTime time = QDateTime::fromMSecsSinceEpoch(info.mes);
    layout->addWidget(new QLabel(QString("时间: %1").arg(time.toString("HH:mm:ss")), card));

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

    // =============================================
    // 1. 布局比例设置
    // =============================================
    ui->gridLayout_Main->setColumnStretch(0, 1); // 左侧列表
    ui->gridLayout_Main->setColumnStretch(1, 3); // 中间地图 (给大一点空间)
    ui->gridLayout_Main->setColumnStretch(2, 1); // 右侧控制

    ui->gridLayout_Main->setRowStretch(0, 4); // 主界面高度
    ui->gridLayout_Main->setRowStretch(1, 1); // 日志高度

    // =============================================
    // 2. 初始化雷达控件 (天地图)
    // =============================================
    m_radar = new RadarView(this);
    ui->groupBox_Radar->layout()->addWidget(m_radar);
    if (ui->widgetRadar) ui->widgetRadar->hide();

    // =============================================
    // 3. 重构左侧布局 (朴素风格: 按钮 + Stack)
    // =============================================

    // 3.0 清除旧控件
    if (ui->groupBox_Targets->layout()) {
        QLayoutItem *item;
        while ((item = ui->groupBox_Targets->layout()->takeAt(0)) != nullptr) {
            if(item->widget()) delete item->widget();
            delete item;
        }
        delete ui->groupBox_Targets->layout();
    }

    // 3.1 左侧主布局
    QVBoxLayout *leftMainLayout = new QVBoxLayout(ui->groupBox_Targets);
    leftMainLayout->setContentsMargins(5, 5, 5, 5);
    leftMainLayout->setSpacing(5);

    // --- 3.2 顶部切换按钮组 ---
    QHBoxLayout *btnLayout = new QHBoxLayout();

    m_btnSwitchDrone = new QPushButton("无人机列表", this);
    m_btnSwitchDrone->setFixedHeight(30); // 稍微设个高度，其他保持默认灰

    m_btnSwitchImage = new QPushButton("图传/频谱", this);
    m_btnSwitchImage->setFixedHeight(30);

    btnLayout->addWidget(m_btnSwitchDrone);
    btnLayout->addWidget(m_btnSwitchImage);

    // 告警红点 (放在最右边)
    btnLayout->addStretch();
    m_lblAlertCount = new QLabel("0", this);
    // 红点还是要稍微醒目一点，不然看不见
    m_lblAlertCount->setStyleSheet("background-color: red; color: white; border-radius: 8px; padding: 2px 6px; font-weight: bold; font-size: 11px;");
    m_lblAlertCount->hide();
    btnLayout->addWidget(m_lblAlertCount);

    leftMainLayout->addLayout(btnLayout);

    // --- 3.3 内容堆叠区域 (StackedWidget) ---
    m_leftStack = new QStackedWidget(this);
    leftMainLayout->addWidget(m_leftStack);

    // [页面 0] 无人机列表
    QScrollArea *scrollDrone = new QScrollArea();
    scrollDrone->setWidgetResizable(true);
    scrollDrone->setStyleSheet("background-color: #1E1E1E; border: none;");

    m_droneListContainer = new QWidget();
    m_droneListLayout = new QVBoxLayout(m_droneListContainer);
    m_droneListLayout->setAlignment(Qt::AlignTop);
    m_droneListLayout->setSpacing(5); // 列表项间距
    scrollDrone->setWidget(m_droneListContainer);

    m_leftStack->addWidget(scrollDrone);

    // [页面 1] 图传/频谱列表
    QScrollArea *scrollImage = new QScrollArea();
    scrollImage->setWidgetResizable(true);

    scrollImage->setStyleSheet("background-color: #1E1E1E; border: none;");

    m_imageListContainer = new QWidget();
    m_imageListLayout = new QVBoxLayout(m_imageListContainer);
    m_imageListLayout->setAlignment(Qt::AlignTop);
    m_imageListLayout->setSpacing(5);
    scrollImage->setWidget(m_imageListContainer);

    m_leftStack->addWidget(scrollImage);

    // --- 3.4 按钮点击切换逻辑 ---
    connect(m_btnSwitchDrone, &QPushButton::clicked, this, [this](){
        m_leftStack->setCurrentIndex(0);
    });
    connect(m_btnSwitchImage, &QPushButton::clicked, this, [this](){
        m_leftStack->setCurrentIndex(1);
    });

    // =============================================
    // 4. 重构右侧布局 (统一面板: 诱骗在上，干扰在下)
    // =============================================

    // A. 隐藏旧控件
    ui->btnAutoMode->hide();
    ui->btnSpoof->hide();
    if (ui->line) ui->line->hide();
    ui->groupBox_Control->hide();

    // B. 创建右侧总容器
    QWidget *rightPanel = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);

    // C. 顶部 Header (系统状态 + 自动模式)
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

    // =============================================
    // D. 构建统一的 "作战控制" 面板
    // =============================================
    QGroupBox *controlGroup = new QGroupBox("作战控制", this);
    controlGroup->setStyleSheet(
        "QGroupBox { border: 1px solid #444; border-radius: 5px; margin-top: 20px; font-weight: bold; color: #00FF00; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; }"
        );
    QVBoxLayout *ctrlLayout = new QVBoxLayout(controlGroup);
    ctrlLayout->setContentsMargins(15, 25, 15, 15);
    ctrlLayout->setSpacing(10);

    // --- PART 1: 诱骗模式 (SPOOF) ---
    QLabel *lblSpoofTitle = new QLabel("诱骗模式 (SPOOF)", this);
    lblSpoofTitle->setStyleSheet("color: #00FF00; font-weight: bold; font-size: 13px; margin-bottom: 5px;");
    lblSpoofTitle->setAlignment(Qt::AlignCenter);
    ctrlLayout->addWidget(lblSpoofTitle);

    // 线条 1 (诱骗标题下方)
    QFrame *line1 = new QFrame(this);
    line1->setFrameShape(QFrame::HLine);
    line1->setStyleSheet("color: #444;"); // 实线
    ctrlLayout->addWidget(line1);

    // 复选框
    QString chkStyle = "QCheckBox { color: #CCC; font-size: 14px; spacing: 8px; } QCheckBox::indicator { width: 18px; height: 18px; }";

    m_chkCircle = new QCheckBox("圆周驱离 (默认)", this);
    m_chkCircle->setStyleSheet(chkStyle);
    ctrlLayout->addWidget(m_chkCircle);

    m_chkNorth = new QCheckBox("定向驱离：北 (North)", this);
    m_chkNorth->setStyleSheet(chkStyle);
    ctrlLayout->addWidget(m_chkNorth);

    m_chkEast = new QCheckBox("定向驱离：东 (East)", this);
    m_chkEast->setStyleSheet(chkStyle);
    ctrlLayout->addWidget(m_chkEast);

    m_chkSouth = new QCheckBox("定向驱离：南 (South)", this);
    m_chkSouth->setStyleSheet(chkStyle);
    ctrlLayout->addWidget(m_chkSouth);

    m_chkWest = new QCheckBox("定向驱离：西 (West)", this);
    m_chkWest->setStyleSheet(chkStyle);
    ctrlLayout->addWidget(m_chkWest);

    m_chkCircle->setChecked(true);

    // 执行诱骗按钮
    m_btnExecuteSpoof = new QPushButton("开启诱骗 (EXECUTE)", this);
    m_btnExecuteSpoof->setCheckable(true);
    m_btnExecuteSpoof->setMinimumHeight(45);
    m_btnExecuteSpoof->setStyleSheet(
        "QPushButton { background-color: #444; color: white; border-radius: 5px; font-weight: bold; font-size: 14px; margin-top: 5px; }"
        "QPushButton:checked { background-color: #FF4500; color: white; }"
        );
    ctrlLayout->addWidget(m_btnExecuteSpoof);

    // --- PART 2: 干扰/压制 (JAMMING) ---
    ctrlLayout->addSpacing(25); // 间距

    // 标题
    QLabel *lblJamTitle = new QLabel("干扰/压制 (JAMMING)", this);
    lblJamTitle->setStyleSheet("color: #FFA500; font-weight: bold; font-size: 13px; margin-bottom: 5px;");
    lblJamTitle->setAlignment(Qt::AlignCenter);
    ctrlLayout->addWidget(lblJamTitle);

    // 线条 2 (干扰标题下方)
    QFrame *line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet("color: #444;"); // 实线
    ctrlLayout->addWidget(line2);

    // 按钮组
    ui->btnRelayControl->setParent(controlGroup);
    ui->btnRelayControl->setMinimumHeight(40);
    ctrlLayout->addWidget(ui->btnRelayControl);

    ui->btnJammerConfig->setParent(controlGroup);
    ui->btnJammerConfig->setMinimumHeight(40);
    ctrlLayout->addWidget(ui->btnJammerConfig);

    ui->btnJammer->setParent(controlGroup);
    ui->btnJammer->setMinimumHeight(40);
    ctrlLayout->addWidget(ui->btnJammer);

    ctrlLayout->addStretch();

    rightLayout->addWidget(controlGroup);
    ui->gridLayout_Main->addWidget(rightPanel, 0, 2, 2, 1);

    // =============================================
    // 5. 初始化信号连接
    // =============================================
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

// 互斥逻辑：CheckBox 版本
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
