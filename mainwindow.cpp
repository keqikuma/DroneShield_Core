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

    // 图片索引 (可选显示)
    // layout->addWidget(createInfoRow("图标Idx:", QString::number(info.img)));

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

    // 添加弹簧，把后面内容顶到最右边
    headerLayout->addStretch();

    // "告警数量" 文字提示
    QLabel *lblAlertTip = new QLabel("告警数量:", this);
    lblAlertTip->setStyleSheet("color: #AAAAAA; font-size: 12px; font-weight: bold; margin-right: 5px;");
    headerLayout->addWidget(lblAlertTip);

    // 红色数字胶囊
    m_lblAlertCount = new QLabel("0", this);
    m_lblAlertCount->setStyleSheet(
        "QLabel {"
        "  background-color: #FF0000;"
        "  color: white;"
        "  border-radius: 9px;"       // 圆形/椭圆背景
        "  padding: 0px 6px;"         // 左右内边距
        "  font-weight: bold;"
        "  font-size: 12px;"
        "  min-height: 18px;"         // 固定高度
        "  min-width: 14px;"          // 最小宽度
        "  qproperty-alignment: AlignCenter;" // 文字居中
        "}"
        );
    m_lblAlertCount->hide(); // 默认隐藏，有数据才显示

    headerLayout->addWidget(m_lblAlertCount);

    // 设置边距，防止贴边太紧
    headerLayout->setContentsMargins(0, 0, 5, 0);
    leftMainLayout->addLayout(headerLayout);

    // 3.3 Tab 切换页
    QTabWidget *tabWidget = new QTabWidget(this);
    // Tab 样式优化
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
    m_droneListLayout->setAlignment(Qt::AlignTop); // 顶部对齐
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
    m_imageListLayout->setAlignment(Qt::AlignTop); // 顶部对齐
    m_imageListLayout->setContentsMargins(0, 0, 0, 0);
    m_imageListLayout->setSpacing(5);
    scrollImage->setWidget(m_imageListContainer);

    tabWidget->addTab(scrollImage, "图传/频谱");

    leftMainLayout->addWidget(tabWidget);

    // =============================================
    // 4. 重构右侧布局 (保持原有的 ToggleSwitch 逻辑)
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

    // C. 顶部 Header (系统状态 + 自动模式)
    QWidget *headerWidget = new QWidget(this);
    headerWidget->setStyleSheet("background-color: #1E1E1E; border-radius: 8px;");
    headerWidget->setFixedHeight(60);
    QHBoxLayout *headerTopLayout = new QHBoxLayout(headerWidget);
    headerTopLayout->setContentsMargins(15, 0, 15, 0);

    // 移动系统状态标签
    ui->verticalLayout_3->removeWidget(ui->label_SystemStatus);
    ui->label_SystemStatus->setParent(headerWidget);
    headerTopLayout->addWidget(ui->label_SystemStatus);
    headerTopLayout->addStretch();

    // 自动模式标签与开关
    QLabel *lblAuto = new QLabel("自动接管模式", this);
    lblAuto->setStyleSheet("font-weight: bold; font-size: 14px; color: #E0E0E0;");
    headerTopLayout->addWidget(lblAuto);
    headerTopLayout->addSpacing(10);

    m_autoSwitch = new ToggleSwitch(this);
    m_autoSwitch->setFixedSize(50, 28);
    headerTopLayout->addWidget(m_autoSwitch);

    rightLayout->addWidget(headerWidget);

    // D. 处理 Control Box
    // 移除最大宽度限制，设为 Expanding
    ui->groupBox_Control->setMinimumWidth(0);
    ui->groupBox_Control->setMaximumWidth(16777215);
    QSizePolicy policy = ui->groupBox_Control->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    policy.setVerticalPolicy(QSizePolicy::Expanding);
    ui->groupBox_Control->setSizePolicy(policy);
    rightLayout->addWidget(ui->groupBox_Control, 1);

    // E. 放入主布局
    ui->gridLayout_Main->addWidget(rightPanel, 0, 2, 2, 1);

    // F. 构建诱骗面板 (Spoof Panel)
    QWidget *spoofPanel = new QWidget(this);
    QVBoxLayout *spoofLayout = new QVBoxLayout(spoofPanel);
    spoofLayout->setContentsMargins(0, 5, 0, 15);
    spoofLayout->setSpacing(8);

    // F1. 标题
    QLabel *lblSpoofTitle = new QLabel("诱骗模式 (SPOOF)", this);
    lblSpoofTitle->setStyleSheet("color: #00FF00; font-weight: bold; font-size: 13px;");
    lblSpoofTitle->setAlignment(Qt::AlignCenter);
    spoofLayout->addWidget(lblSpoofTitle);

    QFrame *line1 = new QFrame(this);
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    line1->setStyleSheet("color: #444;");
    spoofLayout->addWidget(line1);

    // F2. 创建 5 个模式开关
    auto createRow = [this, spoofLayout](QString text, ToggleSwitch*& ptr) {
        QWidget *rowWidget = new QWidget(this);
        QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(10, 2, 10, 2);
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

    m_tsCircle->setChecked(true); // 默认选中

    // F3. 执行按钮
    m_btnExecuteSpoof = new QPushButton("开启诱骗 (EXECUTE)", this);
    m_btnExecuteSpoof->setCheckable(true);
    m_btnExecuteSpoof->setMinimumHeight(40);
    m_btnExecuteSpoof->setStyleSheet(
        "QPushButton { background-color: #444; color: white; border-radius: 5px; font-weight: bold; font-size: 14px; margin-top: 10px; }"
        "QPushButton:checked { background-color: #FF4500; color: white; }"
        );
    spoofLayout->addWidget(m_btnExecuteSpoof);

    QFrame *line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    line2->setStyleSheet("color: #444; margin-top: 5px;");
    spoofLayout->addWidget(line2);

    // F4. 插入到 Control GroupBox
    if (ui->verticalLayout_3) {
        ui->verticalLayout_3->insertWidget(0, spoofPanel);
        spoofPanel->show();
    }

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

// 核心：更新无人机列表卡片
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

    // 3. 更新雷达显示
    QList<RadarTarget> radarTargets;
    for (const auto &d : drones) {
        RadarTarget t;
        t.id = d.uav_id;
        t.distance = d.distance;
        // 注意：这里假设 azimuth 是以正北为 0 度的真实方位
        t.angle = d.azimuth;
        radarTargets.append(t);
    }
    if (m_radar) m_radar->updateTargets(radarTargets);

    // 4. 更新状态文字
    if (drones.isEmpty()) {
        ui->label_SystemStatus->setText("系统状态: 扫描中...");
        ui->label_SystemStatus->setStyleSheet("color: #00ff00;");
    } else {
        ui->label_SystemStatus->setText(QString("系统状态: 发现威胁 (%1)").arg(drones.size()));
        ui->label_SystemStatus->setStyleSheet("color: #ff0000; font-weight: bold; font-size: 14px;");
    }
}

// 核心：更新图传列表卡片
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

// 核心：更新告警红点
void MainWindow::slotUpdateAlertCount(int count)
{
    m_lblAlertCount->setText(QString::number(count));
    if (count > 0) {
        m_lblAlertCount->show();
    } else {
        m_lblAlertCount->hide();
    }
}

// 核心：更新设备自身定位 (传给雷达)
void MainWindow::slotUpdateDevicePos(double lat, double lng)
{
    // 如果 RadarView 有设置中心点的接口，可以在这里调用
    // 例如: m_radar->setCenterLocation(lat, lng);
    // 目前仅做日志打印，证明数据通了
    // qDebug() << "[GPS] Device Position Updated:" << lat << lng;
}

// ============================================================================
// 交互逻辑 (保持不变)
// ============================================================================

void MainWindow::handleSpoofToggleMutex(ToggleSwitch* current)
{
    if (current->isChecked()) {
        if (current != m_tsCircle) m_tsCircle->setChecked(false);
        if (current != m_tsNorth)  m_tsNorth->setChecked(false);
        if (current != m_tsEast)   m_tsEast->setChecked(false);
        if (current != m_tsSouth)  m_tsSouth->setChecked(false);
        if (current != m_tsWest)   m_tsWest->setChecked(false);
    } else {
        if (!m_tsCircle->isChecked() && !m_tsNorth->isChecked() &&
            !m_tsEast->isChecked() && !m_tsSouth->isChecked() && !m_tsWest->isChecked()) {
            m_tsCircle->setChecked(true);
        }
    }
}

void MainWindow::initConnections()
{
    connect(m_autoSwitch, &ToggleSwitch::toggled, this, [this](bool checked){
        emit sigSetAutoMode(checked);
        slotUpdateLog(checked ? ">>> [模式] 切换至自动接管 (AUTO)" : ">>> [模式] 切换至手动操作 (MANUAL)");
    });

    connect(m_tsCircle, &ToggleSwitch::toggled, this, [this](){ handleSpoofToggleMutex(m_tsCircle); });
    connect(m_tsNorth,  &ToggleSwitch::toggled, this, [this](){ handleSpoofToggleMutex(m_tsNorth); });
    connect(m_tsEast,   &ToggleSwitch::toggled, this, [this](){ handleSpoofToggleMutex(m_tsEast); });
    connect(m_tsSouth,  &ToggleSwitch::toggled, this, [this](){ handleSpoofToggleMutex(m_tsSouth); });
    connect(m_tsWest,   &ToggleSwitch::toggled, this, [this](){ handleSpoofToggleMutex(m_tsWest); });

    connect(m_btnExecuteSpoof, &QPushButton::toggled, this, [this](bool checked){
        if (checked) {
            m_btnExecuteSpoof->setText("停止诱骗 (STOP)");
            if (m_tsCircle->isChecked()) { emit sigManualSpoofCircle(); slotUpdateLog(">>> [指令] 启动诱骗 -> 圆周模式"); }
            else if (m_tsNorth->isChecked()) { emit sigManualSpoofNorth(); slotUpdateLog(">>> [指令] 启动诱骗 -> 北 (N)"); }
            else if (m_tsEast->isChecked())  { emit sigManualSpoofEast();  slotUpdateLog(">>> [指令] 启动诱骗 -> 东 (E)"); }
            else if (m_tsSouth->isChecked()) { emit sigManualSpoofSouth(); slotUpdateLog(">>> [指令] 启动诱骗 -> 南 (S)"); }
            else if (m_tsWest->isChecked())  { emit sigManualSpoofWest();  slotUpdateLog(">>> [指令] 启动诱骗 -> 西 (W)"); }
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
