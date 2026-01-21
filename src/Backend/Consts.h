/**
 *  consts.h
 *      Created on: 2026年1月21日
 *          Author: rustinchen
 */

#ifndef CONSTS_H
#define CONSTS_H

#include <QString>

namespace Config {
// 模拟/真实 IP 配置
#ifdef SIMULATION_MODE
const QString IP_BOARD_15   = "127.0.0.1";
const int PORT_BOARD_15     = 7001;
const QString IP_LURE_LOGIC = "127.0.0.1";
const int PORT_LURE_LOGIC   = 9099;
#else
const QString IP_BOARD_15   = "192.168.1.185";
const int PORT_BOARD_15     = 7;
const QString IP_LURE_LOGIC = "192.168.10.203";
const int PORT_LURE_LOGIC   = 9099;
#endif

const QString LURE_SKEY = "a57502fcdc4e7412";

// 默认衰减值 (参考 config.zip/power.ini)
// 对应 atten29, atten30 (单发) 和 atten31, atten32 (配合)
const int ATTEN_29 = 0;
const int ATTEN_30 = 0;
const int ATTEN_31 = 0;
const int ATTEN_32 = 0;

// 波形文件路径前缀
// 在 Mac 上如果是构建目录，建议用绝对路径或者资源文件，这里暂时写相对路径
const QString WAVE_PATH_GPS = "gps_1581M_1.dat";
const QString WAVE_PATH_GLO = "glonass_1603M_1.dat";
}
#endif // CONSTS_H
