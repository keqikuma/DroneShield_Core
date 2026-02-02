#ifndef CONSTS_H
#define CONSTS_H

#include <QString>

namespace Config {

// --- 默认网络配置 (出厂硬编码设置) ---
#ifdef SIMULATION_MODE
// 模拟模式配置
const QString DEFAULT_SPOOF_IP   = "127.0.0.1";
const int     DEFAULT_SPOOF_PORT = 9099;

const QString DEFAULT_DETECT_IP  = "127.0.0.1";
const int     DEFAULT_DETECT_PORT= 8090;

const QString DEFAULT_JAMMER_IP  = "127.0.0.1";
const int     DEFAULT_JAMMER_PORT= 8090;

const QString DEFAULT_RELAY_IP   = "127.0.0.1";
const int     DEFAULT_RELAY_PORT = 2000;

const QString DEFAULT_AMP_IP    = "127.0.0.1";
const int     DEFAULT_AMP_PORT  = 4196;
const QString DEFAULT_AMP2_IP   = "127.0.0.1";
const int     DEFAULT_AMP2_PORT = 4197;
#else
// === 真实硬件默认配置 ===

// 1. 诱骗设备 (UDP)
const QString DEFAULT_SPOOF_IP   = "192.168.10.230";
const int     DEFAULT_SPOOF_PORT = 9099;

// 2. 侦测设备 (WebSocket)
const QString DEFAULT_DETECT_IP  = "192.178.1.12";
const int     DEFAULT_DETECT_PORT= 8090;

// 3. 干扰/写频 (HTTP) - 通常与侦测是同一个板卡
const QString DEFAULT_JAMMER_IP  = "192.178.1.12";
const int     DEFAULT_JAMMER_PORT= 8090;

// 4. 压制继电器 (TCP)
const QString DEFAULT_RELAY_IP   = "192.168.10.221";
const int     DEFAULT_RELAY_PORT = 4196;

// 5. 功放控制 (AmpDevice / AmpDevice2) - 写频开启前对两路功放发开启，关闭时发关闭
const QString DEFAULT_AMP_IP     = "192.168.1.253";
const int     DEFAULT_AMP_PORT   = 4196;
const QString DEFAULT_AMP2_IP   = "192.168.1.153";
const int     DEFAULT_AMP2_PORT  = 4196;
#endif

// 基站经纬度
constexpr double BASE_LAT = 31.2304;
constexpr double BASE_LON = 121.4737;

constexpr double DEG_TO_RAD = 0.017453292519943295769236907684886;
constexpr double EARTH_RADIUS = 6378137.0;

const QString LURE_SKEY = "a57502fcdc4e7412";
}
#endif // CONSTS_H
