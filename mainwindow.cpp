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
// 卡片创建函数
// ============================================================================
QWidget* MainWindow::createDroneCard(const DroneInfo &info) {
    QFrame *card = new QFrame(this);
    card->setFrameShape(QFrame::Box);
    card->setFrameShadow(QFrame::Plain);

    card->setStyleSheet(
        "QFrame { background-color: #2D2D2D; color: #FFFFFF; border: 1px solid #505050; border-radius: 4px; }"
        "QLabel { border: none; background: transparent; }"
        );

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setSpacing(4);
    layout->setContentsMargins(10, 10, 10, 10);

    QLabel *lblTitle = new QLabel(QString("机型: %1").arg(info.model_name), card);
    QFont titleFont;
    titleFont.setBold(true);
    titleFont.setPointSize(14);
    lblTitle->setFont(titleFont);
    lblTitle->setStyleSheet("color: #FFD700;");
    layout->addWidget(lblTitle);

    auto addRow = [&](QString label, QString value) {
        QLabel *lbl = new QLabel(QString("%1: %2").arg(label).arg(value), card);
        lbl->setStyleSheet("font-size: 12px;");
        layout->addWidget(lbl);
    };

    addRow("ID", info.uav_id);
    addRow("距离", QString::number(info.distance, 'f', 1) + " m");
    addRow("方位角", QString::number(info.azimuth, 'f', 1) + "°");
    addRow("频率", QString::number(info.freq, 'f', 1) + " MHz");
    addRow("高度", QString::number(info.height, 'f', 1) + " m");
    addRow("无人机坐标", QString("%1, %2").arg(info.uav_lat, 0, 'f', 6).arg(info.uav_lng, 0, 'f', 6));
    addRow("飞手坐标", QString("%1, %2").arg(info.pilot_lat, 0, 'f', 6).arg(info.pilot_lng, 0, 'f', 6));
    addRow("飞手距离", QString::number(info.pilot_distance, 'f', 1) + " m");
    addRow("速度", info.velocity);
    addRow("UUID", info.uuid);

    return card;
}

QWidget* MainWindow::createImageCard(const ImageInfo &info) {
    QFrame *card = new QFrame(this);
    card->setFrameShape(QFrame::Box);
    card->setFrameShadow(QFrame::Plain);

    card->setStyleSheet(
        "QFrame { background-color: #2D2D2D; color: #FFFFFF; border: 1px solid #505050; border-radius: 4px; }"
        "QLabel { border: none; background: transparent; }"
        );

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setSpacing(4);
    layout->setContentsMargins(10, 10, 10, 10);

    // 根据类型显示不同标题颜色
    // Type 1 = FPV (0x06), Type 0 = Image/Spectrum (0x07)
    QString typeStr = (info.type == 1) ? "信号: FPV (0x06)" : "信号: 图传 (0x07)";
    QLabel *lblTitle = new QLabel(typeStr, card);
    QFont titleFont;
    titleFont.setBold(true);
    titleFont.setPointSize(14);
    lblTitle->setFont(titleFont);

    // FPV用蓝色，图传用绿色区分
    lblTitle->setStyleSheet(info.type == 1 ? "color: #00BFFF;" : "color: #32CD32;");
    layout->addWidget(lblTitle);

    layout->addWidget(new QLabel(QString("ID: %1").arg(info.id), card));
    layout->addWidget(new QLabel(QString("频率: %1 MHz").arg(info.freq, 0, 'f', 1), card));
    layout->addWidget(new QLabel(QString("强度: %1").arg(info.amplitude, 0, 'f', 1), card));

    QDateTime time = QDateTime::fromMSecsSinceEpoch(QDateTime::currentMSecsSinceEpoch());
    layout->addWidget(new QLabel(QString("时间: %1").arg(time.toString("HH:mm:ss")), card));

    return card;
}

// ============================================================================
// 主窗口构造
// ============================================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. 布局
    ui->gridLayout_Main->setColumnStretch(0, 1);
    ui->gridLayout_Main->setColumnStretch(1, 3);
    ui->gridLayout_Main->setColumnStretch(2, 1);
    ui->gridLayout_Main->setRowStretch(0, 4);
    ui->gridLayout_Main->setRowStretch(1, 1);

    // 2. 雷达
    m_radar = new RadarView(this);
    ui->groupBox_Radar->layout()->addWidget(m_radar);
    if (ui->widgetRadar) ui->widgetRadar->hide();

    // 3. 构建左侧 UI (无人机, FPV, 图传)
    setupLeftPanel();

    // 4. 重构右侧布局
    ui->btnAutoMode->hide();
    ui->btnSpoof->hide();
    if (ui->line) ui->line->hide();
    ui->groupBox_Control->hide();

    QWidget *rightPanel = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);

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

    QGroupBox *controlGroup = new QGroupBox("作战控制", this);
    controlGroup->setStyleSheet(
        "QGroupBox { border: 1px solid #444; border-radius: 5px; margin-top: 20px; font-weight: bold; color: #00FF00; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; }"
        );
    QVBoxLayout *ctrlLayout = new QVBoxLayout(controlGroup);
    ctrlLayout->setContentsMargins(15, 25, 15, 15);
    ctrlLayout->setSpacing(10);

    // --- PART 1: 诱骗 ---
    QLabel *lblSpoofTitle = new QLabel("诱骗模式 (SPOOF)", this);
    lblSpoofTitle->setStyleSheet("color: #00FF00; font-weight: bold; font-size: 13px; margin-bottom: 5px;");
    lblSpoofTitle->setAlignment(Qt::AlignCenter);
    ctrlLayout->addWidget(lblSpoofTitle);

    QFrame *line1 = new QFrame(this);
    line1->setFrameShape(QFrame::HLine);
    line1->setStyleSheet("color: #444;");
    ctrlLayout->addWidget(line1);

    QString chkStyle = "QCheckBox { color: #CCC; font-size: 14px; spacing: 8px; } QCheckBox::indicator { width: 18px; height: 18px; }";
    m_chkCircle = new QCheckBox("圆周驱离 (默认)", this); m_chkCircle->setStyleSheet(chkStyle); ctrlLayout->addWidget(m_chkCircle);
    m_chkNorth = new QCheckBox("定向驱离：北 (North)", this); m_chkNorth->setStyleSheet(chkStyle); ctrlLayout->addWidget(m_chkNorth);
    m_chkEast = new QCheckBox("定向驱离：东 (East)", this); m_chkEast->setStyleSheet(chkStyle); ctrlLayout->addWidget(m_chkEast);
    m_chkSouth = new QCheckBox("定向驱离：南 (South)", this); m_chkSouth->setStyleSheet(chkStyle); ctrlLayout->addWidget(m_chkSouth);
    m_chkWest = new QCheckBox("定向驱离：西 (West)", this); m_chkWest->setStyleSheet(chkStyle); ctrlLayout->addWidget(m_chkWest);
    m_chkCircle->setChecked(true);

    m_btnExecuteSpoof = new QPushButton("开启诱骗 (EXECUTE)", this);
    m_btnExecuteSpoof->setCheckable(true);
    m_btnExecuteSpoof->setMinimumHeight(45);
    m_btnExecuteSpoof->setStyleSheet(
        "QPushButton { background-color: #444; color: white; border-radius: 5px; font-weight: bold; font-size: 14px; margin-top: 5px; }"
        "QPushButton:checked { background-color: #FF4500; color: white; }"
        );
    ctrlLayout->addWidget(m_btnExecuteSpoof);

    // --- PART 2: 干扰 ---
    ctrlLayout->addSpacing(25);
    QLabel *lblJamTitle = new QLabel("干扰/压制 (JAMMING)", this);
    lblJamTitle->setStyleSheet("color: #FFA500; font-weight: bold; font-size: 13px; margin-bottom: 5px;");
    lblJamTitle->setAlignment(Qt::AlignCenter);
    ctrlLayout->addWidget(lblJamTitle);

    QFrame *line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet("color: #444;");
    ctrlLayout->addWidget(line2);

    ui->btnRelayControl->setParent(controlGroup); ui->btnRelayControl->setMinimumHeight(40); ctrlLayout->addWidget(ui->btnRelayControl);
    ui->btnJammerConfig->setParent(controlGroup); ui->btnJammerConfig->setMinimumHeight(40); ctrlLayout->addWidget(ui->btnJammerConfig);
    ui->btnJammer->setParent(controlGroup); ui->btnJammer->setMinimumHeight(40); ctrlLayout->addWidget(ui->btnJammer);

    ctrlLayout->addStretch();
    rightLayout->addWidget(controlGroup);
    ui->gridLayout_Main->addWidget(rightPanel, 0, 2, 2, 1);

    // 5. 初始化
    m_deviceManager = new DeviceManager(this);

    m_uiTimer = new QTimer(this);
    m_uiTimer->setInterval(500);
    connect(m_uiTimer, &QTimer::timeout, this, &MainWindow::onUiRefreshTimeout);
    m_uiTimer->start();

    initConnections();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ============================================================================
// 初始化左侧面板
// ============================================================================
void MainWindow::setupLeftPanel()
{
    if (ui->groupBox_Targets->layout()) {
        QLayoutItem *item;
        while ((item = ui->groupBox_Targets->layout()->takeAt(0)) != nullptr) {
            if(item->widget()) delete item->widget();
            delete item;
        }
        delete ui->groupBox_Targets->layout();
    }

    QVBoxLayout *mainLayout = new QVBoxLayout(ui->groupBox_Targets);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    // 1. 顶部按钮 (无人机 | FPV | 图传)
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(2);

    m_btnDrone = new QPushButton("无人机 (0)", this);
    m_btnFPV   = new QPushButton("FPV (0)", this);    // 对应 0x06
    m_btnImage = new QPushButton("图传 (0)", this);   // 对应 0x07

    m_btnDrone->setFixedHeight(35);
    m_btnFPV->setFixedHeight(35);
    m_btnImage->setFixedHeight(35);

    m_btnDrone->setCheckable(true);
    m_btnFPV->setCheckable(true);
    m_btnImage->setCheckable(true);
    m_btnDrone->setChecked(true); // 默认选第一个

    btnLayout->addWidget(m_btnDrone);
    btnLayout->addWidget(m_btnFPV);
    btnLayout->addWidget(m_btnImage);

    mainLayout->addLayout(btnLayout);

    // 2. 堆叠窗口
    m_leftStack = new QStackedWidget(this);
    mainLayout->addWidget(m_leftStack);

    auto createListContainer = [&](QWidget*& container, QVBoxLayout*& layout) {
        QScrollArea *scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setStyleSheet("background-color: #1E1E1E; border: none;");

        container = new QWidget();
        container->setStyleSheet("background: transparent;");

        layout = new QVBoxLayout(container);
        layout->setAlignment(Qt::AlignTop);
        layout->setSpacing(5);

        scroll->setWidget(container);
        m_leftStack->addWidget(scroll);
    };

    createListContainer(m_droneContainer, m_droneLayout); // Index 0: 无人机
    createListContainer(m_fpvContainer,   m_fpvLayout);   // Index 1: FPV
    createListContainer(m_imageContainer, m_imageLayout); // Index 2: 图传

    // 3. 切换逻辑
    auto updateBtnState = [&](int index) {
        m_leftStack->setCurrentIndex(index);
        m_btnDrone->setChecked(index == 0);
        m_btnFPV->setChecked(index == 1);
        m_btnImage->setChecked(index == 2);
    };

    connect(m_btnDrone, &QPushButton::clicked, this, [=](){ updateBtnState(0); });
    connect(m_btnFPV,   &QPushButton::clicked, this, [=](){ updateBtnState(1); });
    connect(m_btnImage, &QPushButton::clicked, this, [=](){ updateBtnState(2); });
}

// ============================================================================
// 数据处理槽 (分流到不同的 Cache)
// ============================================================================

void MainWindow::slotUpdateLog(const QString &msg)
{
    QString timeStr = QDateTime::currentDateTime().toString("[HH:mm:ss] ");
    ui->textLog->append(timeStr + msg);
}

void MainWindow::slotUpdateDroneList(const QList<DroneInfo> &drones)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (const auto &d : drones) {
        m_droneCache.insert(d.uav_id, d);
        m_lastSeenTime.insert(d.uav_id, now);
    }
}

void MainWindow::slotUpdateImageList(const QList<ImageInfo> &images)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (const auto &img : images) {
        // 根据 Type 分流
        if (img.type == 1) {
            // Type 1 = FPV (0x06) -> 存入 FPV Cache
            m_fpvCache.insert(img.id, img);
            m_lastSeenTime.insert(img.id, now);
        } else {
            // Type 0 = Image/Spectrum (0x07) -> 存入 Image Cache
            m_imageCache.insert(img.id, img);
            m_lastSeenTime.insert(img.id, now);
        }
    }
}

void MainWindow::slotUpdateAlertCount(int count)
{
    Q_UNUSED(count);
}

void MainWindow::slotUpdateDevicePos(double lat, double lng)
{
    if (m_radar) m_radar->setCenterPosition(lat, lng);
}

// ============================================================================
// 定时刷新逻辑
// ============================================================================
void MainWindow::onUiRefreshTimeout()
{
    // 1. 清理过期
    cleanExpiredTargets();

    // 2. 更新按钮文字
    m_btnDrone->setText(QString("无人机 (%1)").arg(m_droneCache.size()));
    m_btnFPV->setText(QString("FPV (%1)").arg(m_fpvCache.size()));
    m_btnImage->setText(QString("图传 (%1)").arg(m_imageCache.size()));

    // 3. 刷新当前页
    int idx = m_leftStack->currentIndex();

    if (idx == 0) { // 无人机
        QLayoutItem *child;
        while ((child = m_droneLayout->takeAt(0)) != nullptr) {
            if (child->widget()) delete child->widget();
            delete child;
        }
        for (const auto &info : m_droneCache) {
            m_droneLayout->addWidget(createDroneCard(info));
        }

        QList<RadarTarget> mapTargets;
        for (const auto &d : m_droneCache) {
            RadarTarget t;
            t.id = d.uav_id;
            t.lat = d.uav_lat;
            t.lng = d.uav_lng;
            t.angle = d.azimuth;
            mapTargets.append(t);
        }
        if (m_radar) m_radar->updateTargets(mapTargets);

        if (m_droneCache.isEmpty()) {
            ui->label_SystemStatus->setText("系统状态: 扫描中...");
            ui->label_SystemStatus->setStyleSheet("color: #00ff00;");
        } else {
            ui->label_SystemStatus->setText(QString("系统状态: 发现威胁 (%1)").arg(m_droneCache.size()));
            ui->label_SystemStatus->setStyleSheet("color: #ff0000; font-weight: bold; font-size: 14px;");
        }
    }
    else if (idx == 1) { // FPV
        QLayoutItem *child;
        while ((child = m_fpvLayout->takeAt(0)) != nullptr) {
            if (child->widget()) delete child->widget();
            delete child;
        }
        for (const auto &info : m_fpvCache) {
            m_fpvLayout->addWidget(createImageCard(info));
        }
    }
    else if (idx == 2) { // 图传
        QLayoutItem *child;
        while ((child = m_imageLayout->takeAt(0)) != nullptr) {
            if (child->widget()) delete child->widget();
            delete child;
        }
        for (const auto &info : m_imageCache) {
            m_imageLayout->addWidget(createImageCard(info));
        }
    }
}

void MainWindow::cleanExpiredTargets()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    int timeout = 4000;

    auto itD = m_droneCache.begin();
    while (itD != m_droneCache.end()) {
        if (now - m_lastSeenTime.value(itD.key(), 0) > timeout) {
            m_lastSeenTime.remove(itD.key());
            itD = m_droneCache.erase(itD);
        } else ++itD;
    }

    auto itF = m_fpvCache.begin();
    while (itF != m_fpvCache.end()) {
        if (now - m_lastSeenTime.value(itF.key(), 0) > timeout) {
            m_lastSeenTime.remove(itF.key());
            itF = m_fpvCache.erase(itF);
        } else ++itF;
    }

    auto itI = m_imageCache.begin();
    while (itI != m_imageCache.end()) {
        if (now - m_lastSeenTime.value(itI.key(), 0) > timeout) {
            m_lastSeenTime.remove(itI.key());
            itI = m_imageCache.erase(itI);
        } else ++itI;
    }
}

// ============================================================================
// 信号连接
// ============================================================================
void MainWindow::handleSpoofCheckBoxMutex(QCheckBox* current)
{
    if (current->isChecked()) {
        if (current != m_chkCircle) m_chkCircle->setChecked(false);
        if (current != m_chkNorth)  m_chkNorth->setChecked(false);
        if (current != m_chkEast)   m_chkEast->setChecked(false);
        if (current != m_chkSouth)  m_chkSouth->setChecked(false);
        if (current != m_chkWest)   m_chkWest->setChecked(false);
    } else {
        if (!m_chkCircle->isChecked() && !m_chkNorth->isChecked() &&
            !m_chkEast->isChecked() && !m_chkSouth->isChecked() && !m_chkWest->isChecked()) {
            m_chkCircle->setChecked(true);
        }
    }
}

void MainWindow::initConnections()
{
    connect(m_deviceManager, &DeviceManager::sigLogMessage, this, &MainWindow::slotUpdateLog);
    connect(m_deviceManager, &DeviceManager::sigDroneList,  this, &MainWindow::slotUpdateDroneList);
    connect(m_deviceManager, &DeviceManager::sigImageList,  this, &MainWindow::slotUpdateImageList);
    connect(m_deviceManager, &DeviceManager::sigAlertCount, this, &MainWindow::slotUpdateAlertCount);
    connect(m_deviceManager, &DeviceManager::sigSelfPosition, this, &MainWindow::slotUpdateDevicePos);

    connect(m_autoSwitch, &ToggleSwitch::toggled, this, [this](bool checked){
        emit sigSetAutoMode(checked);
        slotUpdateLog(checked ? ">>> [模式] 切换至自动接管 (AUTO)" : ">>> [模式] 切换至手动操作 (MANUAL)");
    });

    connect(m_chkCircle, &QCheckBox::toggled, this, [this](){ handleSpoofCheckBoxMutex(m_chkCircle); });
    connect(m_chkNorth,  &QCheckBox::toggled, this, [this](){ handleSpoofCheckBoxMutex(m_chkNorth); });
    connect(m_chkEast,   &QCheckBox::toggled, this, [this](){ handleSpoofCheckBoxMutex(m_chkEast); });
    connect(m_chkSouth,  &QCheckBox::toggled, this, [this](){ handleSpoofCheckBoxMutex(m_chkSouth); });
    connect(m_chkWest,   &QCheckBox::toggled, this, [this](){ handleSpoofCheckBoxMutex(m_chkWest); });

    connect(m_btnExecuteSpoof, &QPushButton::toggled, this, [this](bool checked){
        if (checked) {
            m_btnExecuteSpoof->setText("停止诱骗 (STOP)");
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
