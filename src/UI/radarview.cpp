#include "radarview.h"
#include <QPainter>
#include <QMouseEvent>
#include <QtMath>
#include <QDebug>
#include <QCoreApplication>

const int TILE_SIZE = 256;
const double PI = 3.14159265358979323846;

RadarView::RadarView(QWidget *parent) : QWidget(parent)
{
    // =============================================================
    // 1. 配置
    // =============================================================
    m_tiandituKey = "67fffe9b5c71def384e7e1a2ee641a7e"; // <--- 请替换 KEY
    m_layerType = "img_w"; // 卫星图

    // 2. 初始视角 (西安大华)
    m_zoomLevel = 15;
    m_centerLat = 34.218146;
    m_centerLng = 108.834316;
    m_wheelAccumulator = 0;
    m_isDragging = false;

    // 3. 初始化缓存目录 (区分 Windows 和 Mac)
    initCacheDirectory();

    // 4. 网络初始化
    m_netManager = new QNetworkAccessManager(this);
    connect(m_netManager, &QNetworkAccessManager::finished,
            this, &RadarView::onTileDownloaded);

    // 5. 设置背景
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(20, 20, 20));
    setPalette(pal);
    setAutoFillBackground(true);
}

RadarView::~RadarView() {}

// =========================================================
// 核心：缓存目录初始化 (区分系统)
// =========================================================
void RadarView::initCacheDirectory()
{
    QString basePath;

#ifdef Q_OS_WIN
    // --- Windows 策略 ---
    // 方案A (推荐): 存放在 AppData/Local/DroneShield/Cache
    // 路径示例: C:/Users/Admin/AppData/Local/DroneShield/tiles_cache
    // basePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

    // 方案B (便携): 存放在 exe 同级目录下 (如果不安装，只是拷贝运行用这个)
    basePath = QCoreApplication::applicationDirPath() + "/LocalCache";
#elif defined(Q_OS_MAC)
    // --- Mac 策略 ---
    // 路径示例: /Users/Rustin/Library/Caches/DroneShield/tiles_cache
    basePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#else
    // Linux 等其他系统
    basePath = QDir::homePath() + "/.droneshield_cache";
#endif

    m_diskCachePath = basePath + "/tiles_cache";

    // 创建目录
    QDir dir(m_diskCachePath);
    if (!dir.exists()) {
        if(dir.mkpath(".")) {
            qDebug() << "[System] 缓存目录创建成功:" << m_diskCachePath;
        } else {
            qWarning() << "[Error] 缓存目录创建失败!";
        }
    } else {
        qDebug() << "[System] 使用现有缓存目录:" << m_diskCachePath;
    }
}

QString RadarView::getTileFilePath(int x, int y, int z)
{
    // 生成文件名: img_w_15_123_456.png
    return QString("%1/%2_%3_%4_%5.png")
        .arg(m_diskCachePath)
        .arg(m_layerType)
        .arg(z).arg(x).arg(y);
}

// =========================================================
// 绘图事件
// =========================================================
void RadarView::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 1. 计算中心瓦片位置
    QPointF centerTilePos = latLonToTile(m_centerLat, m_centerLng, m_zoomLevel);

    // 2. 计算屏幕覆盖范围
    int width = this->width();
    int height = this->height();

    double halfW = width / 2.0 / TILE_SIZE;
    double halfH = height / 2.0 / TILE_SIZE;

    int startX = floor(centerTilePos.x() - halfW);
    int endX = ceil(centerTilePos.x() + halfW);
    int startY = floor(centerTilePos.y() - halfH);
    int endY = ceil(centerTilePos.y() + halfH);

    // 3. 绘制瓦片
    int maxTile = pow(2, m_zoomLevel);

    for (int x = startX; x <= endX; ++x) {
        for (int y = startY; y <= endY; ++y) {
            int validX = (x % maxTile + maxTile) % maxTile; // 经度循环
            if (y < 0 || y >= maxTile) continue;

            TileCoord coord = {validX, y, m_zoomLevel};
            QPointF screenPos = tileToScreen(QPointF(x, y), centerTilePos);

            // 优先画内存缓存
            if (m_tileCache.contains(coord)) {
                p.drawPixmap(screenPos, m_tileCache[coord]);
            } else {
                // 画网格占位
                p.setPen(QColor(60, 60, 60));
                p.drawRect(screenPos.x(), screenPos.y(), TILE_SIZE, TILE_SIZE);
                // 触发下载
                fetchTile(validX, y, m_zoomLevel);
            }
        }
    }

    // 4. 绘制中心 (本机)
    QPoint centerScreen(width / 2, height / 2);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 255, 0, 200));
    p.drawEllipse(centerScreen, 8, 8);
    p.setPen(Qt::white);
    p.drawText(centerScreen + QPoint(12, 5), "本机");

    // 5. 绘制无人机目标
    for (const auto &target : m_targets) {
        QPointF tPos = latLonToTile(target.lat, target.lng, m_zoomLevel);
        QPointF sPos = tileToScreen(tPos, centerTilePos);

        p.save();
        p.translate(sPos);

        // 绘制目标点 (红色)
        p.setBrush(Qt::red);
        p.setPen(Qt::white);
        p.drawEllipse(QPoint(0,0), 6, 6);

        // 绘制 ID
        p.setPen(Qt::white);
        p.drawText(10, 5, target.id);

        p.restore();
    }

    // 版权信息
    p.setPen(Qt::lightGray);
    p.drawText(rect().bottomRight() - QPoint(120, 10), "天地图 Tianditu");
}

// =========================================================
// 瓦片获取 (内存 -> 磁盘 -> 网络)
// =========================================================
void RadarView::fetchTile(int x, int y, int z)
{
    TileCoord coord = {x, y, z};
    if (m_pendingTiles.contains(coord)) return;

    // A. 查本地磁盘
    QString path = getTileFilePath(x, y, z);
    if (QFile::exists(path)) {
        QPixmap pix;
        if (pix.load(path)) {
            m_tileCache.insert(coord, pix);
            update();
            return;
        }
    }

    // B. 发起网络请求
    m_pendingTiles.append(coord);
    QString url = getTileUrl(x, y, z);
    QNetworkRequest request((QUrl(url)));
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");

    QNetworkReply *reply = m_netManager->get(request);
    reply->setProperty("x", x);
    reply->setProperty("y", y);
    reply->setProperty("z", z);
}

QString RadarView::getTileUrl(int x, int y, int z)
{
    // 天地图 XYZ 接口
    int serverNode = (x + y) % 8;
    return QString("http://t%1.tianditu.gov.cn/DataServer?T=%2&x=%3&y=%4&l=%5&tk=%6")
        .arg(serverNode).arg(m_layerType).arg(x).arg(y).arg(z).arg(m_tiandituKey);
}

void RadarView::onTileDownloaded(QNetworkReply *reply)
{
    reply->deleteLater();
    int x = reply->property("x").toInt();
    int y = reply->property("y").toInt();
    int z = reply->property("z").toInt();
    m_pendingTiles.removeOne({x, y, z});

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QPixmap pix;
        if (pix.loadFromData(data)) {
            // 存内存
            m_tileCache.insert({x, y, z}, pix);
            // 存磁盘
            QString path = getTileFilePath(x, y, z);
            QFile file(path);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(data);
                file.close();
            }
            update();
        }
    }
}

// =========================================================
// 交互与数学计算
// =========================================================
void RadarView::updateTargets(const QList<RadarTarget> &targets) {
    m_targets = targets;
    update();
}

void RadarView::setCenterPosition(double lat, double lng) {
    if (lat != 0 && lng != 0) {
        m_centerLat = lat;
        m_centerLng = lng;
        update();
    }
}

QPointF RadarView::latLonToTile(double lat, double lng, int zoom) {
    double n = pow(2.0, zoom);
    double tileX = ((lng + 180.0) / 360.0) * n;
    double latRad = lat * PI / 180.0;
    double tileY = (1.0 - asinh(tan(latRad)) / PI) / 2.0 * n;
    return QPointF(tileX, tileY);
}

QPointF RadarView::tileToScreen(QPointF tilePos, QPointF centerTilePos) {
    double cx = width() / 2.0;
    double cy = height() / 2.0;
    return QPointF(cx + (tilePos.x() - centerTilePos.x()) * TILE_SIZE,
                   cy + (tilePos.y() - centerTilePos.y()) * TILE_SIZE);
}

void RadarView::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastMousePos = event->pos();
    }
}

void RadarView::mouseMoveEvent(QMouseEvent *event) {
    if (m_isDragging) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();

        // 像素 -> 经纬度反算
        double n = pow(2.0, m_zoomLevel);
        QPointF centerTile = latLonToTile(m_centerLat, m_centerLng, m_zoomLevel);

        double newTileX = centerTile.x() - delta.x() / (double)TILE_SIZE;
        double newTileY = centerTile.y() - delta.y() / (double)TILE_SIZE;

        m_centerLng = (newTileX / n) * 360.0 - 180.0;
        double latRad = atan(sinh(PI * (1 - 2 * newTileY / n)));
        m_centerLat = latRad * 180.0 / PI;

        update();
    }
}

void RadarView::wheelEvent(QWheelEvent *event) {
    m_wheelAccumulator += event->angleDelta().y();
    // 阈值设为 120 (标准鼠标滚轮一格)
    if (abs(m_wheelAccumulator) >= 120) {
        int steps = m_wheelAccumulator / 120;
        int newZoom = m_zoomLevel + steps;

        if (newZoom >= 1 && newZoom <= 18) {
            m_zoomLevel = newZoom;
            // 缩放时重置缓存可能会导致闪烁，不建议 m_tileCache.clear()
            update();
        }
        m_wheelAccumulator = 0;
    }
    event->accept();
}
