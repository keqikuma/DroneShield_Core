#include "configloader.h"
#include "../Backend/Consts.h"
#include <QCoreApplication>
#include <QDebug>
#include <QFile>

ConfigLoader::ConfigLoader(QObject *parent) : QObject{parent}
{
    initDefaults();
}

void ConfigLoader::initDefaults()
{
    // 配置文件路径: 运行目录/config.ini
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);

    // 辅助 Lambda：读取配置，如果不存在则写入默认值
    auto loadOrSet = [&](const QString &group, const QString &defIp, int defPort) -> NetConfig {
        settings.beginGroup(group);
        if (!settings.contains("IP")) settings.setValue("IP", defIp);
        if (!settings.contains("Port")) settings.setValue("Port", defPort);

        NetConfig cfg;
        cfg.ip = settings.value("IP", defIp).toString();
        cfg.port = settings.value("Port", defPort).toInt();
        settings.endGroup();
        return cfg;
    };

    // 1. 诱骗
    m_spoof = loadOrSet("SpoofDevice", Config::DEFAULT_SPOOF_IP, Config::DEFAULT_SPOOF_PORT);

    // 2. 侦测
    m_detect = loadOrSet("DetectionDevice", Config::DEFAULT_DETECT_IP, Config::DEFAULT_DETECT_PORT);

    // 3. 干扰
    m_jammer = loadOrSet("JammerDevice", Config::DEFAULT_JAMMER_IP, Config::DEFAULT_JAMMER_PORT);

    // 4. 继电器
    m_relay = loadOrSet("RelayDevice", Config::DEFAULT_RELAY_IP, Config::DEFAULT_RELAY_PORT);

    // 5. 功放 (两路：写频开启/关闭时对两路都发指令)
    m_amp  = loadOrSet("AmpDevice",  Config::DEFAULT_AMP_IP,  Config::DEFAULT_AMP_PORT);
    m_amp2 = loadOrSet("AmpDevice2", Config::DEFAULT_AMP2_IP, Config::DEFAULT_AMP2_PORT);

    settings.sync(); // 确保写入磁盘

    qDebug() << "[Config] 配置加载完毕 -> " << configPath;
    qDebug() << "  Spoof :" << m_spoof.ip << ":" << m_spoof.port;
    qDebug() << "  Detect:" << m_detect.ip << ":" << m_detect.port;
    qDebug() << "  Jammer:" << m_jammer.ip << ":" << m_jammer.port;
    qDebug() << "  Relay :" << m_relay.ip << ":" << m_relay.port;
    qDebug() << "  Amp   :" << m_amp.ip << ":" << m_amp.port;
    qDebug() << "  Amp2  :" << m_amp2.ip << ":" << m_amp2.port;
}

NetConfig ConfigLoader::getSpoofConfig() const { return m_spoof; }
NetConfig ConfigLoader::getDetectConfig() const { return m_detect; }
NetConfig ConfigLoader::getJammerConfig() const { return m_jammer; }
NetConfig ConfigLoader::getRelayConfig() const { return m_relay; }
NetConfig ConfigLoader::getAmpConfig() const { return m_amp; }
NetConfig ConfigLoader::getAmp2Config() const { return m_amp2; }
