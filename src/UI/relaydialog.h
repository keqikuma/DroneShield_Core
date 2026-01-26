#ifndef RELAYDIALOG_H
#define RELAYDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QPushButton>
#include <QList>

class RelayDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RelayDialog(QWidget *parent = nullptr);

signals:
    // 立即触发控制信号
    void sigControlChannel(int channel, bool on);
    void sigControlAll(bool on);

private:
    void setupUi();
    QList<QCheckBox*> m_checks;
};

#endif // RELAYDIALOG_H
