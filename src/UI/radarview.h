#ifndef RADARVIEW_H
#define RADARVIEW_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHash>
#include <QPixmap>
#include <QMap>

// 简单的瓦片索引结构
struct TileCoord {
    int x;
    int y;
    int z;

    // 用于 QHash 的比较
    bool operator==(const TileCoord &other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

// 全局哈希函数，用于 TileCoord
inline uint qHash(const TileCoord &key, uint seed = 0) {
    return qHash(QString("%1-%2-%3").arg(key.x).arg(key.y).arg(key.z), seed);
}

// 目标结构体
struct RadarTarget {
    QString id;
    double lat; // 改为经纬度
    double lng;
    double angle; // 依然保留，用于画箭头朝向
};

class RadarView : public QWidget
{
    Q_OBJECT
public:
    explicit RadarView(QWidget *parent = nullptr);
    ~RadarView();

    // 更新接口
    void updateTargets(const QList<RadarTarget> &targets);
    // 设置地图中心（设备自身位置）
    void setCenterPosition(double lat, double lng);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void onTileDownloaded(QNetworkReply *reply);

private:
    // --- 天地图配置 ---
    QString m_tiandituKey; // 你的 Key
    QString m_layerType;   // "img_w" (卫星) 或 "vec_w" (矢量)

    // --- 地图状态 ---
    int m_zoomLevel;       // 缩放级别 (1-18)
    double m_centerLat;    // 中心纬度
    double m_centerLng;    // 中心经度

    // --- 缓存 ---
    QNetworkAccessManager *m_netManager;
    QHash<TileCoord, QPixmap> m_tileCache; // 内存缓存瓦片
    QList<TileCoord> m_pendingTiles;       // 正在下载的瓦片

    // --- 数据 ---
    QList<RadarTarget> m_targets; // 无人机列表

    // --- 交互 ---
    QPoint m_lastMousePos;
    bool m_isDragging;

    // --- 核心数学函数 ---
    // 经纬度转瓦片坐标
    QPointF latLonToTile(double lat, double lng, int zoom);
    // 瓦片坐标转屏幕像素
    QPointF tileToScreen(QPointF tilePos, QPointF centerTilePos);
    // 下载瓦片
    void fetchTile(int x, int y, int z);
    // 获取瓦片 URL
    QString getTileUrl(int x, int y, int z);
};

#endif // RADARVIEW_H
