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
#else
    // 任务书要求的 IP
    const QString DEFAULT_SPOOF_IP   = "192.178.3.200";
    const int     DEFAULT_SPOOF_PORT = 9099;
    // --- Linux 主控板配置 (侦测/写频) ---
    const QString LINUX_MAIN_IP   = "192.178.1.12";
    const int     LINUX_PORT      = 8090;
#endif

    const QString LURE_SKEY = "a57502fcdc4e7412";

    // 默认波形文件路径
    const QString WAVE_PATH_GPS = "gps_1581M_1.dat";
    const QString WAVE_PATH_GLO = "glonass_1603M_1.dat";
}
#endif // CONSTS_H
