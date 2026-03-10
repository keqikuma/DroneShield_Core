#ifndef RELAYDIALOG_H
#define RELAYDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QPushButton>
#include <QList>

class ToggleSwitch;

class RelayDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RelayDialog(QWidget *parent = nullptr);

signals:
    void sigControlChannel(int channel, bool on);
    void sigControlAll(bool on);

private:
    void setupUi();
    QList<QCheckBox*> m_checks;
    ToggleSwitch *m_masterSwitch = nullptr; // 定频总开关：ON 时勾选才下发，OFF 时发全关
};

#endif // RELAYDIALOG_H
