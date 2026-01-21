/**
 *  consts.h
 *      Created on: 2026年1月21日
 *          Author: rustinchen
 */

#ifndef CONSTS_H
#define CONSTS_H

#include <QString>
#include <QMap>

namespace Config {
// 开启模拟模式时，IP 指向本机
#ifdef SIMULATION_MODE
    const QString IP_BOARD_15   = "127.0.0.1"; // 模拟板卡15
    const int PORT_BOARD_15     = 7001;        // 模拟TCP端口

    const QString IP_LURE_LOGIC = "127.0.0.1"; // 模拟诱骗逻辑单元
    const int PORT_LURE_LOGIC   = 9099;        // 模拟UDP端口
#else
    const QString IP_BOARD_15   = "192.168.1.185"; // 真实IP (参考 ip.ini)
    const int PORT_BOARD_15     = 7;

    const QString IP_LURE_LOGIC = "192.168.10.203"; // 真实IP (参考 udp.js)
    const int PORT_LURE_LOGIC   = 9099;
#endif

    // 诱骗专用 Key (参考 udp.js var SKEY)
    const QString LURE_SKEY = "a57502fcdc4e7412";
}
#endif // CONSTS_H
