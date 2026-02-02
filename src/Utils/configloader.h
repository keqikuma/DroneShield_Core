#ifndef CONFIGLOADER_H
#define CONFIGLOADER_H

#include <QObject>
#include <QSettings>
#include <QString>

struct NetConfig {
    QString ip;
    int port;
};

class ConfigLoader : public QObject
{
    Q_OBJECT
public:
    explicit ConfigLoader(QObject *parent = nullptr);

    // 获取各设备配置
    NetConfig getSpoofConfig() const;
    NetConfig getDetectConfig() const;
    NetConfig getJammerConfig() const;
    NetConfig getRelayConfig() const;
    NetConfig getAmpConfig() const;
    NetConfig getAmp2Config() const;

private:
    void initDefaults(); // 初始化/读取配置

    NetConfig m_spoof;
    NetConfig m_detect;
    NetConfig m_jammer;
    NetConfig m_relay;
    NetConfig m_amp;
    NetConfig m_amp2;
};

#endif // CONFIGLOADER_H

