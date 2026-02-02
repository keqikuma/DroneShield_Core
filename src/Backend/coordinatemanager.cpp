#include "coordinatemanager.h"
#include <QDebug>

CoordinateManager::CoordinateManager(QObject *parent)
    : QObject(parent), m_isLocked(false),
    m_realBaseLat(0), m_realBaseLng(0),
    m_snapBaseLat(0), m_snapBaseLng(0)
{
}

void CoordinateManager::lock()
{
    QMutexLocker locker(&m_mutex);
    if (!m_isLocked) {
        m_isLocked = true;
        // 生成快照
        m_snapBaseLat = m_realBaseLat;
        m_snapBaseLng = m_realBaseLng;
        m_snapDrones = m_realDrones;
        // qDebug() << "[CoordMgr] 数据已锁定 (Snapshot Created)";
    }
}

void CoordinateManager::unlock()
{
    QMutexLocker locker(&m_mutex);
    if (m_isLocked) {
        m_isLocked = false;
        // qDebug() << "[CoordMgr] 数据已解锁 (Live Mode)";
    }
}

void CoordinateManager::updateBasePosition(double lat, double lng)
{
    QMutexLocker locker(&m_mutex);

    // 1. 基础有效性检查
    if (lat < 0.1 || lng < 0.1) return;

    // 2. 异常过滤 (Anti-Jitter)
    // 如果之前有有效坐标，且新坐标距离上次坐标超过 1000米 (基站不可能瞬移)，则认为是干扰数据
    if (m_realBaseLat > 1.0) {
        double dist = calculateDistance(lat, lng, m_realBaseLat, m_realBaseLng);
        if (dist > 1000.0) {
            // qDebug() << "[CoordMgr] 忽略异常基站坐标跳变，距离差:" << dist;
            return;
        }
    }

    // 3. 更新实时数据
    m_realBaseLat = lat;
    m_realBaseLng = lng;
}

void CoordinateManager::updateDroneList(const QList<DroneInfo> &drones)
{
    QMutexLocker locker(&m_mutex);
    m_realDrones = drones;
}

void CoordinateManager::getEffectiveBasePosition(double &lat, double &lng)
{
    QMutexLocker locker(&m_mutex);
    if (m_isLocked) {
        lat = m_snapBaseLat;
        lng = m_snapBaseLng;
    } else {
        lat = m_realBaseLat;
        lng = m_realBaseLng;
    }
}

QList<DroneInfo> CoordinateManager::getEffectiveDroneList()
{
    QMutexLocker locker(&m_mutex);
    if (m_isLocked) {
        return m_snapDrones;
    } else {
        return m_realDrones;
    }
}

// 静态辅助函数
double CoordinateManager::calculateDistance(double lat1, double lng1, double lat2, double lng2)
{
    if (qAbs(lat1) < 0.0001 || qAbs(lng1) < 0.0001 ||
        qAbs(lat2) < 0.0001 || qAbs(lng2) < 0.0001) {
        return 0.0;
    }
    double earthRadius = 6378137.0;
    double radLat1 = qDegreesToRadians(lat1);
    double radLat2 = qDegreesToRadians(lat2);
    double a = radLat1 - radLat2;
    double b = qDegreesToRadians(lng1) - qDegreesToRadians(lng2);
    double s = 2 * asin(sqrt(pow(sin(a/2), 2) +
                             cos(radLat1) * cos(radLat2) * pow(sin(b/2), 2)));
    return s * earthRadius;
}
