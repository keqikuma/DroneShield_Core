#ifndef CONFIGLOADER_H
#define CONFIGLOADER_H

#include <QObject>
#include <QSettings>
#include <QString>

class ConfigLoader : public QObject
{
    Q_OBJECT
public:
    explicit ConfigLoader(QObject *parent = nullptr);

    // 读取配置
    QString getSpoofIp() const;
    int getSpoofPort() const;

private:
    void initDefaults(); // 如果文件不存在，写入默认值
    QString m_spoofIp;
    int m_spoofPort;
};

#endif // CONFIGLOADER_H
