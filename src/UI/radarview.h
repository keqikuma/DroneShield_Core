#ifndef RADARVIEW_H
#define RADARVIEW_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHash>
#include <QPixmap>
#include <QMap>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

// --- 瓦片索引结构 ---
struct TileCoord {
    int x;
    int y;
    int z;

    // 用于 QHash 的比较
    bool operator==(const TileCoord &other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

// 全局哈希函数
inline uint qHash(const TileCoord &key, uint seed = 0) {
    return qHash(QString("%1-%2-%3").arg(key.x).arg(key.y).arg(key.z), seed);
}

// --- 雷达目标结构 ---
struct RadarTarget {
    QString id;
    double lat;
    double lng;
    double angle;
};

class RadarView : public QWidget
{
    Q_OBJECT
public:
    explicit RadarView(QWidget *parent = nullptr);
    ~RadarView();

    // 更新目标接口
    void updateTargets(const QList<RadarTarget> &targets);
    // 设置本机中心点
    void setCenterPosition(double lat, double lng);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void onTileDownloaded(QNetworkReply *reply);

private:
    // --- 配置 ---
    QString m_tiandituKey;
    QString m_layerType;

    // --- 状态 ---
    int m_zoomLevel;
    double m_centerLat;
    double m_centerLng;
    int m_wheelAccumulator; // Mac 滚轮平滑处理

    // --- 缓存与网络 ---
    QNetworkAccessManager *m_netManager;
    QHash<TileCoord, QPixmap> m_tileCache; // 内存缓存
    QList<TileCoord> m_pendingTiles;       // 正在下载队列
    QString m_diskCachePath;               // 本地磁盘缓存目录

    // --- 数据 ---
    QList<RadarTarget> m_targets;

    // --- 交互 ---
    QPoint m_lastMousePos;
    bool m_isDragging;

    // --- 核心函数 ---
    void initCacheDirectory(); // 初始化缓存目录
    QString getTileFilePath(int x, int y, int z); // 获取本地文件路径
    QString getTileUrl(int x, int y, int z);      // 获取网络URL
    void fetchTile(int x, int y, int z);          // 获取瓦片(本地->网络)

    // --- 数学计算 ---
    QPointF latLonToTile(double lat, double lng, int zoom);
    QPointF tileToScreen(QPointF tilePos, QPointF centerTilePos);
};

#endif // RADARVIEW_H
