#ifndef COORDINATEMANAGER_H
#define COORDINATEMANAGER_H

#include <QObject>
#include <QMutex>
#include <QMap>
#include <QtMath>
#include "DataStructs.h" // 包含 DroneInfo 定义

class CoordinateManager : public QObject
{
    Q_OBJECT
public:
    explicit CoordinateManager(QObject *parent = nullptr);

    // --- 数据输入 (来自驱动) ---
    void updateBasePosition(double lat, double lng);
    void updateDroneList(const QList<DroneInfo> &drones);

    // --- 状态控制 ---
    void lock();   // 开启诱骗前调用：锁定当前数据
    void unlock(); // 关闭诱骗后调用：恢复实时更新

    // --- 数据获取 (业务逻辑调用) ---
    // 获取基站坐标 (如果是锁定状态，返回快照；否则返回实时)
    void getEffectiveBasePosition(double &lat, double &lng);

    // 获取无人机列表 (如果是锁定状态，返回快照列表；否则返回实时列表)
    QList<DroneInfo> getEffectiveDroneList();

    // 辅助：计算距离
    static double calculateDistance(double lat1, double lng1, double lat2, double lng2);

private:
    bool m_isLocked;

    // 实时数据
    double m_realBaseLat;
    double m_realBaseLng;
    QList<DroneInfo> m_realDrones;

    // 快照数据 (锁定那一刻的数据)
    double m_snapBaseLat;
    double m_snapBaseLng;
    QList<DroneInfo> m_snapDrones;

    mutable QMutex m_mutex;
};

#endif // COORDINATEMANAGER_H
