#ifndef JAMMERCONFDIALOG_H
#define JAMMERCONFDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>

// 定义一个简单的结构体传输配置
struct JammerBoardConfig {
    bool enabled;
    int freqType; // 1=板卡1, 2=板卡2
    int startFreq;
    int endFreq;
};

class JammerConfigDialog : public QDialog
{
    Q_OBJECT
public:
    explicit JammerConfigDialog(QWidget *parent = nullptr);

    // 获取用户配置
    QList<JammerBoardConfig> getConfigs() const;

private:
    void setupUi();

    // 板卡 1 控件
    QCheckBox *m_chkBoard1;
    QLineEdit *m_editStart1;
    QLineEdit *m_editEnd1;

    // 板卡 2 控件
    QCheckBox *m_chkBoard2;
    QLineEdit *m_editStart2;
    QLineEdit *m_editEnd2;
};

#endif // JAMMERCONFDIALOG_H
