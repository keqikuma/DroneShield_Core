#ifndef DATASTRUCTS_H
#define DATASTRUCTS_H

#include <QString>
#include <QList>

// 1. 设备自身定位 (对应 info 事件)
struct DevicePosition {
    double lat = 0.0;
    double lng = 0.0;
    bool isValid = false;
};

// 2. 无人机信息 (对应 droneStatus 事件)
struct DroneInfo {
    QString uav_id;         // 序列号
    QString model_name;     // 机型
    double distance = 0.0;  // 距离 (米) - 【修复】初始化
    double azimuth = 0.0;   // 方位角 (度)
    double uav_lat = 0.0;   // 无人机纬度
    double uav_lng = 0.0;   // 无人机经度
    double height = 0.0;    // 高度
    double freq = 0.0;      // 频率 (MHz)
    QString velocity;       // 速度描述
    double pilot_lat = 0.0; // 飞手纬度
    double pilot_lng = 0.0; // 飞手经度
    double pilot_distance = 0.0; // 飞手距离

    bool whiteList = false; // 【关键修复】必须初始化为 false！否则自动防御会失效

    QString uuid;           // 追踪ID
    int img = 0;            // 图标索引
    QString type;           // 固定标识
};

// 3. 图传/频谱信息 (对应 imageStatus 事件)
struct ImageInfo {
    QString id;             // 唯一标识 (频率_类型)
    double freq = 0.0;      // 频率 (MHz)
    double amplitude = 0.0; // 信号强度
    int type = 0;           // 1=FPV(图传), 0=UAV(数传信号)
    long long mes = 0;      // 时间戳
    int first = 0;          // 轮次标识
};

#endif // DATASTRUCTS_H
