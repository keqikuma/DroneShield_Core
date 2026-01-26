#include "relaydialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
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

    // --- 单路控制 ---
    QGroupBox *grpSingle = new QGroupBox("单频段控制 (手动)", this);
    QGridLayout *grid = new QGridLayout(grpSingle);

    for (int i = 1; i <= 7; ++i) {
        QCheckBox *chk = new QCheckBox(QString("通道 %1").arg(i), this);
        m_checks.append(chk);
        // 布局：每行放 3 个
        grid->addWidget(chk, (i-1)/3, (i-1)%3);

        // 点击即触发
        connect(chk, &QCheckBox::toggled, this, [this, i](bool checked){
            emit sigControlChannel(i, checked);
        });
    }
    mainLayout->addWidget(grpSingle);

    // --- 总控 ---
    QGroupBox *grpMaster = new QGroupBox("全频段压制", this);
    QHBoxLayout *hbox = new QHBoxLayout(grpMaster);

    QPushButton *btnAllOn = new QPushButton("🔥 全开 (ALL ON)", this);
    btnAllOn->setStyleSheet("background-color: #8B0000; color: white; font-weight: bold; padding: 10px;");

    QPushButton *btnAllOff = new QPushButton("🛡️ 全关 (ALL OFF)", this);
    btnAllOff->setStyleSheet("background-color: #2F4F4F; color: white; padding: 10px;");

    hbox->addWidget(btnAllOn);
    hbox->addWidget(btnAllOff);
    mainLayout->addWidget(grpMaster);

    // 连接总控
    connect(btnAllOn, &QPushButton::clicked, this, [this](){
        // 将所有勾选框设为 true，并发送全开指令
        for(auto c : m_checks) c->setChecked(true);
        emit sigControlAll(true);
    });

    connect(btnAllOff, &QPushButton::clicked, this, [this](){
        for(auto c : m_checks) c->setChecked(false);
        emit sigControlAll(false);
    });
}
