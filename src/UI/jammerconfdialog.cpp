#include "jammerconfdialog.h"
#include <QFormLayout>
#include <QSettings>

JammerConfigDialog::JammerConfigDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("手动干扰参数配置");
    setMinimumWidth(350);
    setupUi();
}

void JammerConfigDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // --- 板卡 1 设置 ---
    QGroupBox *grp1 = new QGroupBox("板卡 1 (低频段)", this);
    QVBoxLayout *vbox1 = new QVBoxLayout(grp1);
    m_chkBoard1 = new QCheckBox("启用此板卡", this);
    m_chkBoard1->setChecked(true);

    QFormLayout *form1 = new QFormLayout();
    m_editStart1 = new QLineEdit("900", this);
    m_editEnd1 = new QLineEdit("920", this);
    form1->addRow("起始频率 (MHz):", m_editStart1);
    form1->addRow("结束频率 (MHz):", m_editEnd1);

    vbox1->addWidget(m_chkBoard1);
    vbox1->addLayout(form1);
    mainLayout->addWidget(grp1);

    // --- 板卡 2 设置 ---
    QGroupBox *grp2 = new QGroupBox("板卡 2 (高频段)", this);
    QVBoxLayout *vbox2 = new QVBoxLayout(grp2);
    m_chkBoard2 = new QCheckBox("启用此板卡", this);
    m_chkBoard2->setChecked(true); // 默认全选

    QFormLayout *form2 = new QFormLayout();
    m_editStart2 = new QLineEdit("900", this);
    m_editEnd2 = new QLineEdit("920", this);
    form2->addRow("起始频率 (MHz):", m_editStart2);
    form2->addRow("结束频率 (MHz):", m_editEnd2);

    vbox2->addWidget(m_chkBoard2);
    vbox2->addLayout(form2);
    mainLayout->addWidget(grp2);

    // --- 按钮 ---
    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(btnBox);

    // 记忆：恢复上次的 start/end freq 与启用状态
    QSettings s;
    if (s.contains("JammerConfig/Board1Start")) {
        m_editStart1->setText(s.value("JammerConfig/Board1Start").toString());
        m_editEnd1->setText(s.value("JammerConfig/Board1End").toString());
    }
    if (s.contains("JammerConfig/Board2Start")) {
        m_editStart2->setText(s.value("JammerConfig/Board2Start").toString());
        m_editEnd2->setText(s.value("JammerConfig/Board2End").toString());
    }
    if (s.contains("JammerConfig/Board1Enabled"))
        m_chkBoard1->setChecked(s.value("JammerConfig/Board1Enabled").toBool());
    if (s.contains("JammerConfig/Board2Enabled"))
        m_chkBoard2->setChecked(s.value("JammerConfig/Board2Enabled").toBool());
}

void JammerConfigDialog::accept()
{
    QSettings s;
    s.setValue("JammerConfig/Board1Start", m_editStart1->text().trimmed());
    s.setValue("JammerConfig/Board1End", m_editEnd1->text().trimmed());
    s.setValue("JammerConfig/Board2Start", m_editStart2->text().trimmed());
    s.setValue("JammerConfig/Board2End", m_editEnd2->text().trimmed());
    s.setValue("JammerConfig/Board1Enabled", m_chkBoard1->isChecked());
    s.setValue("JammerConfig/Board2Enabled", m_chkBoard2->isChecked());
    QDialog::accept();
}

QList<JammerBoardConfig> JammerConfigDialog::getConfigs() const
{
    QList<JammerBoardConfig> list;

    if (m_chkBoard1->isChecked()) {
        JammerBoardConfig c1;
        c1.enabled = true;
        c1.freqType = 1;
        c1.startFreq = m_editStart1->text().toInt();
        c1.endFreq = m_editEnd1->text().toInt();
        list.append(c1);
    }

    if (m_chkBoard2->isChecked()) {
        JammerBoardConfig c2;
        c2.enabled = true;
        c2.freqType = 2;
        c2.startFreq = m_editStart2->text().toInt();
        c2.endFreq = m_editEnd2->text().toInt();
        list.append(c2);
    }

    return list;
}
