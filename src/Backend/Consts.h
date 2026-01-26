/**
 *  consts.h
 *      Created on: 2026年1月21日
 *          Author: rustinchen
 */

#ifndef CONSTS_H
#define CONSTS_H

#include <QString>

namespace Config {

// --- 默认配置 (当 config.ini 不存在时使用) ---
#ifdef SIMULATION_MODE
    const QString DEFAULT_SPOOF_IP   = "127.0.0.1";
    const int     DEFAULT_SPOOF_PORT = 9099;

    const QString LINUX_MAIN_IP   = "127.0.0.1";
    const int     LINUX_PORT      = 8090;

    // === 继电器 (压制设备) ===
    // 假设通过串口服务器连接，模拟时用本机
    const QString RELAY_IP = "127.0.0.1";
    const int     RELAY_PORT = 2000;
#else
    // 任务书要求的 IP
    const QString DEFAULT_SPOOF_IP   = "192.178.3.200";
    const int     DEFAULT_SPOOF_PORT = 9099;
    // --- Linux 主控板配置 (侦测/写频) ---
    const QString LINUX_MAIN_IP   = "192.178.1.12";
    const int     LINUX_PORT      = 8090;
    // --- 继电器控制压制 ip ---
    const QString RELAY_IP = "127.0.0.1";
    const int     RELAY_PORT = 2000;
#endif

    // 真实部署时，这里填写反制设备的实际经纬度
    constexpr double BASE_LAT = 31.2304;
    constexpr double BASE_LON = 121.4737;

    constexpr double DEG_TO_RAD = 0.017453292519943295769236907684886;
    constexpr double EARTH_RADIUS = 6378137.0; // 地球半径 (米)

    const QString LURE_SKEY = "a57502fcdc4e7412";

    // 默认波形文件路径
    const QString WAVE_PATH_GPS = "gps_1581M_1.dat";
    const QString WAVE_PATH_GLO = "glonass_1603M_1.dat";
}
#endif // CONSTS_H
