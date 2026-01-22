#include "configloader.h"
#include "../Backend/Consts.h"
#include <QCoreApplication>
#include <QDebug>
#include <QFile>

ConfigLoader::ConfigLoader(QObject *parent)
    : QObject{parent}
{
    initDefaults();
}

void ConfigLoader::initDefaults()
{
    // 配置文件路径: 运行目录/config.ini
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";

    QSettings settings(configPath, QSettings::IniFormat);

    // 检查是否存在 [SpoofDevice] 组，如果不存在则写入默认值
    if (!settings.contains("SpoofDevice/IP")) {
        qDebug() << "[Config] 配置文件不存在或缺失，创建默认配置 -> " << configPath;
        settings.setValue("SpoofDevice/IP", Config::DEFAULT_SPOOF_IP);
        settings.setValue("SpoofDevice/Port", Config::DEFAULT_SPOOF_PORT);
        settings.sync(); // 强制写入磁盘
    }

    // 读取配置到内存
    m_spoofIp = settings.value("SpoofDevice/IP", Config::DEFAULT_SPOOF_IP).toString();
    m_spoofPort = settings.value("SpoofDevice/Port", Config::DEFAULT_SPOOF_PORT).toInt();

    qDebug() << "[Config] 加载诱骗设备配置 IP:" << m_spoofIp << " Port:" << m_spoofPort;
}

QString ConfigLoader::getSpoofIp() const
{
    return m_spoofIp;
}

int ConfigLoader::getSpoofPort() const
{
    return m_spoofPort;
}
