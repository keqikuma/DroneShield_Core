#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QDebug>
#include "Drivers/spoofdriver.h"

// 【新增】定义方向枚举
enum class SpoofDirection {
    North, // 北 (0度)
    East,  // 东 (90度)
    South, // 南 (180度)
    West   // 西 (270度)
};

// 【新增】定义工作模式（为第三阶段自动模式做准备）
enum class SystemMode {
    Manual, // 手动
    Auto    // 自动
};

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager();

    // ==========================================
    // 基础控制
    // ==========================================
    void startSpoofing(double lat, double lon, double alt = 100.0);
    void stopSpoofing();

    // ==========================================
    // 手动模式业务接口
    // ==========================================

    // 手动：开启圆周驱离
    void setManualCircular();

    // 手动：开启定向驱离 (东/南/西/北)
    void setManualDirection(SpoofDirection dir);

    // 切换系统模式 (手动/自动)
    void setSystemMode(SystemMode mode);

private:
    SpoofDriver *m_spoofDriver;
    SystemMode m_currentMode; // 当前系统模式
};

#endif // DEVICEMANAGER_H
