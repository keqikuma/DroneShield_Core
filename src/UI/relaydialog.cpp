#include "relaydialog.h"
#include "toggleswitch.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>

RelayDialog::RelayDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("信号压制控制 (Relay)");
    setMinimumWidth(400);
    setupUi();
}

void RelayDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // --- 定频总开关：ON 时勾选才下发单路指令，OFF 时发全关 ---
    QGroupBox *grpSwitch = new QGroupBox("定频总开关", this);
    QHBoxLayout *switchLayout = new QHBoxLayout(grpSwitch);
    QLabel *lblSwitch = new QLabel("开启后，下方勾选才会生效", this);
    m_masterSwitch = new ToggleSwitch(this);
    m_masterSwitch->setFixedSize(50, 28);
    switchLayout->addWidget(lblSwitch);
    switchLayout->addWidget(m_masterSwitch);
    mainLayout->addWidget(grpSwitch);

    // --- 单路控制（按表格顺序：433, 915, 1.2, 1.5, 2.4, 5.2, 5.8）---
    QGroupBox *grpSingle = new QGroupBox("单频段控制 (手动)", this);
    QGridLayout *grid = new QGridLayout(grpSingle);

    static const char* channelNames[] = { "433", "915", "1.2", "1.5", "2.4", "5.2", "5.8" };
    for (int i = 1; i <= 7; ++i) {
        QString label = QString("通道 %1 (%2)").arg(i).arg(channelNames[i - 1]);
        QCheckBox *chk = new QCheckBox(label, this);
        m_checks.append(chk);
        grid->addWidget(chk, (i-1)/3, (i-1)%3);

        // 仅当定频总开关为 ON 时，勾选变化才下发单路指令
        connect(chk, &QCheckBox::toggled, this, [this, i](bool checked){
            if (m_masterSwitch && m_masterSwitch->isChecked())
                emit sigControlChannel(i, checked);
        });
    }
    mainLayout->addWidget(grpSingle);

    // 定频总开关：ON -> 按当前勾选下发 7 路；OFF -> 发全关
    connect(m_masterSwitch, &ToggleSwitch::toggled, this, [this](bool checked){
        if (checked) {
            for (int i = 1; i <= m_checks.size(); ++i)
                emit sigControlChannel(i, m_checks.at(i - 1)->isChecked());
        } else {
            emit sigControlAll(false);
        }
    });

    // --- 全开/全关 ---
    QGroupBox *grpMaster = new QGroupBox("全频段压制", this);
    QHBoxLayout *hbox = new QHBoxLayout(grpMaster);

    QPushButton *btnAllOn = new QPushButton("全开 (ALL ON)", this);
    btnAllOn->setStyleSheet("background-color: #8B0000; color: white; font-weight: bold; padding: 10px;");

    QPushButton *btnAllOff = new QPushButton("全关 (ALL OFF)", this);
    btnAllOff->setStyleSheet("background-color: #2F4F4F; color: white; padding: 10px;");

    hbox->addWidget(btnAllOn);
    hbox->addWidget(btnAllOff);
    mainLayout->addWidget(grpMaster);

    connect(btnAllOn, &QPushButton::clicked, this, [this](){
        for (QCheckBox *c : m_checks) {
            c->blockSignals(true);
            c->setChecked(true);
            c->blockSignals(false);
        }
        emit sigControlAll(true);
    });

    connect(btnAllOff, &QPushButton::clicked, this, [this](){
        for (QCheckBox *c : m_checks) {
            c->blockSignals(true);
            c->setChecked(false);
            c->blockSignals(false);
        }
        emit sigControlAll(false);
    });
}
