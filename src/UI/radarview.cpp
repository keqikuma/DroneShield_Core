#include "radarview.h"
#include <QPainter>
#include <QMouseEvent>
#include <QtMath>
#include <QDebug>

// 常量
const int TILE_SIZE = 256;
const double PI = 3.14159265358979323846;

RadarView::RadarView(QWidget *parent) : QWidget(parent)
{
    // =============================================================
    // 1. 天地图配置 (关键)
    // =============================================================
    // 请务必替换为你申请的 "浏览器端" Key
    m_tiandituKey = "67fffe9b5c71def384e7e1a2ee641a7e";

    // 图层类型:
    // "img_w" = 卫星影像 (球面墨卡托投影) -> 推荐，更有雷达实战感
    // "vec_w" = 矢量街道 (球面墨卡托投影)
    m_layerType = "img_w";

    // =============================================================
    // 2. 初始地图状态
    // =============================================================
    m_zoomLevel = 15;        // 缩放级别 (1-18)，15级大概能看清街道

    // 初始中心点 (默认为西安大华，与你的 Mock 数据一致)
    // 这样启动程序时，地图就不会显示在海里
    m_centerLat = 34.218146;
    m_centerLng = 108.834316;

    // =============================================================
    // 3. 网络与交互初始化
    // =============================================================
    m_netManager = new QNetworkAccessManager(this);

    // 当瓦片图片下载完成时，触发 onTileDownloaded
    connect(m_netManager, &QNetworkAccessManager::finished,
            this, &RadarView::onTileDownloaded);

    // 设置背景颜色 (当瓦片还没加载出来时显示的底色)
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(20, 20, 20)); // 深灰色
    setPalette(pal);
    setAutoFillBackground(true);

    // 交互状态标记
    m_isDragging = false;
    m_wheelAccumulator = 0; // 初始化为0
}

RadarView::~RadarView() {}

// =========================================================
// 公共接口
// =========================================================

void RadarView::updateTargets(const QList<RadarTarget> &targets)
{
    m_targets = targets;
    update(); // 重绘
}

void RadarView::setCenterPosition(double lat, double lng)
{
    // 只有当位置变化较大，或者从未设置过时才更新，防止无法拖动地图
    // 这里为了演示，我们强制更新为设备中心，实现“跟随模式”
    // 如果你想允许用户拖动查看，可以加个 "autoFollow" 开关
    if (lat != 0 && lng != 0) {
        m_centerLat = lat;
        m_centerLng = lng;
        update();
    }
}

// =========================================================
// 核心绘图逻辑
// =========================================================

void RadarView::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(30, 30, 30)); // 默认深色背景

    // 1. 计算中心点在瓦片坐标系中的位置 (浮点数，比如 x=13000.5, y=6000.5)
    QPointF centerTilePos = latLonToTile(m_centerLat, m_centerLng, m_zoomLevel);

    // 2. 计算屏幕覆盖了哪些瓦片
    int width = this->width();
    int height = this->height();

    // 屏幕中心对应 centerTilePos
    // 计算屏幕左上角对应的瓦片坐标
    double halfW = width / 2.0 / TILE_SIZE;
    double halfH = height / 2.0 / TILE_SIZE;

    int startX = floor(centerTilePos.x() - halfW);
    int endX = ceil(centerTilePos.x() + halfW);
    int startY = floor(centerTilePos.y() - halfH);
    int endY = ceil(centerTilePos.y() + halfH);

    // 3. 绘制地图瓦片
    for (int x = startX; x <= endX; ++x) {
        for (int y = startY; y <= endY; ++y) {
            // 最大瓦片范围限制 (2^zoom)
            int maxTile = pow(2, m_zoomLevel);
            int validX = (x % maxTile + maxTile) % maxTile; // 处理跨越经度180度
            if (y < 0 || y >= maxTile) continue;

            TileCoord coord = {validX, y, m_zoomLevel};

            // 计算这个瓦片在屏幕上的像素位置
            QPointF tileScreenPos = tileToScreen(QPointF(x, y), centerTilePos);

            if (m_tileCache.contains(coord)) {
                p.drawPixmap(tileScreenPos, m_tileCache[coord]);
            } else {
                // 画个网格占位
                p.setPen(QColor(50, 50, 50));
                p.drawRect(tileScreenPos.x(), tileScreenPos.y(), TILE_SIZE, TILE_SIZE);
                // 触发下载
                fetchTile(validX, y, m_zoomLevel);
            }
        }
    }

    // 4. 绘制中心点（设备自身）
    QPoint centerScreen(width / 2, height / 2);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 255, 0, 150)); // 半透明绿
    p.drawEllipse(centerScreen, 10, 10);
    p.setPen(Qt::white);
    p.drawText(centerScreen + QPoint(15, 5), "本机");

    // 5. 绘制无人机目标
    for (const auto &target : m_targets) {
        // 将目标经纬度转为屏幕坐标
        QPointF tPos = latLonToTile(target.lat, target.lng, m_zoomLevel);
        QPointF screenPos = tileToScreen(tPos, centerTilePos);

        // 简单的无人机图标绘制 (红色三角)
        p.save();
        p.translate(screenPos);

        // 绘制目标点
        p.setBrush(Qt::red);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPoint(0,0), 6, 6);

        // 绘制ID文字
        p.setPen(Qt::white);
        p.drawText(10, 5, target.id);

        // 绘制方向箭头 (如果有角度)
        /*
        p.rotate(target.angle);
        QPolygon arrow;
        arrow << QPoint(-5, 5) << QPoint(0, -8) << QPoint(5, 5);
        p.drawPolygon(arrow);
        */

        p.restore();
    }

    // 6. 绘制天地图 Logo/版权 (合规要求)
    p.setPen(Qt::white);
    p.drawText(rect().bottomRight() - QPoint(100, 10), "天地图 Tianditu");
}

// =========================================================
// 数学运算 (Web Mercator)
// =========================================================

QPointF RadarView::latLonToTile(double lat, double lng, int zoom)
{
    double n = pow(2.0, zoom);
    double tileX = ((lng + 180.0) / 360.0) * n;

    double latRad = lat * PI / 180.0;
    double tileY = (1.0 - asinh(tan(latRad)) / PI) / 2.0 * n;

    return QPointF(tileX, tileY);
}

QPointF RadarView::tileToScreen(QPointF tilePos, QPointF centerTilePos)
{
    // 屏幕中心像素
    double cx = width() / 2.0;
    double cy = height() / 2.0;

    // 瓦片相对于中心的偏移量
    double diffX = (tilePos.x() - centerTilePos.x()) * TILE_SIZE;
    double diffY = (tilePos.y() - centerTilePos.y()) * TILE_SIZE;

    return QPointF(cx + diffX, cy + diffY);
}

// =========================================================
// 网络下载
// =========================================================

void RadarView::fetchTile(int x, int y, int z)
{
    TileCoord coord = {x, y, z};
    if (m_pendingTiles.contains(coord)) return; // 已经在下了

    m_pendingTiles.append(coord);

    QString url = getTileUrl(x, y, z);
    QNetworkRequest request((QUrl(url)));

    // 这里的 User-Agent 很重要，有时天地图会拦截非浏览器请求
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");

    QNetworkReply *reply = m_netManager->get(request);
    // 把坐标绑在属性里，方便回调时取回
    reply->setProperty("x", x);
    reply->setProperty("y", y);
    reply->setProperty("z", z);
}

QString RadarView::getTileUrl(int x, int y, int z)
{
    // 1. 随机选择一个服务器节点 (t0 ~ t7) 以分散负载
    int serverNode = (x + y) % 8;

    // 2. 确定图层类型 (T参数)
    // 如果 m_layerType 是 "img"，我们请求 "img_w" (卫星底图)
    // 如果您想看街道，可以改为 "vec_w"
    QString layerParam = m_layerType;
    if (!layerParam.endsWith("_w")) {
        layerParam += "_w"; // 强制使用墨卡托投影 (_w) 以匹配我们的数学公式
    }

    // 3. 拼接 URL (XYZ 格式)
    // 文档参考: http://lbs.tianditu.gov.cn/server/MapService.html
    return QString("http://t%1.tianditu.gov.cn/DataServer?T=%2&x=%3&y=%4&l=%5&tk=%6")
        .arg(serverNode)
        .arg(layerParam)    // 例如 "img_w"
        .arg(x)
        .arg(y)
        .arg(z)
        .arg(m_tiandituKey);
}

void RadarView::onTileDownloaded(QNetworkReply *reply)
{
    reply->deleteLater();
    m_pendingTiles.removeOne({reply->property("x").toInt(), reply->property("y").toInt(), reply->property("z").toInt()});

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Tile download error:" << reply->errorString();
        return;
    }

    QByteArray data = reply->readAll();
    QPixmap pixmap;
    if (pixmap.loadFromData(data)) {
        int x = reply->property("x").toInt();
        int y = reply->property("y").toInt();
        int z = reply->property("z").toInt();

        m_tileCache.insert({x, y, z}, pixmap);
        update(); // 刷新界面显示新瓦片
    }
}

// =========================================================
// 交互事件 (拖拽与缩放)
// =========================================================

void RadarView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastMousePos = event->pos();
    }
}

void RadarView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();

        // 计算移动的经纬度差值 (简化版)
        // 像素 -> 瓦片单位 -> 经纬度
        // 为了平滑体验，通常我们在 paintEvent 里使用 offset，这里简单直接改 centerLat/Lng

        double n = pow(2.0, m_zoomLevel);
        double deltaTileX = -delta.x() / (double)TILE_SIZE;
        double deltaTileY = -delta.y() / (double)TILE_SIZE;

        // 当前中心点的瓦片坐标
        QPointF centerTile = latLonToTile(m_centerLat, m_centerLng, m_zoomLevel);
        double newTileX = centerTile.x() + deltaTileX;
        double newTileY = centerTile.y() + deltaTileY;

        // 瓦片坐标转回经纬度 (逆运算)
        double newLng = (newTileX / n) * 360.0 - 180.0;
        double newLatRad = atan(sinh(PI * (1 - 2 * newTileY / n)));
        double newLat = newLatRad * 180.0 / PI;

        m_centerLat = newLat;
        m_centerLng = newLng;

        update();
    }
}

void RadarView::wheelEvent(QWheelEvent *event)
{
    // 累加滚轮的数值 (Mac上轻轻一滑可能是 +5, +10，普通鼠标一格是 +120)
    m_wheelAccumulator += event->angleDelta().y();

    // 设定阈值：每 120 度算一级 (标准鼠标一格)
    int steps = 0;
    if (m_wheelAccumulator >= 120) {
        steps = 1;
        m_wheelAccumulator = 0; // 清零，或者减去120 (m_wheelAccumulator -= 120) 以保持惯性
    } else if (m_wheelAccumulator <= -120) {
        steps = -1;
        m_wheelAccumulator = 0;
    }

    // 只有当积累够了一级，才执行缩放
    if (steps != 0) {
        int newZoom = m_zoomLevel + steps;

        // 限制范围 1 ~ 18
        if (newZoom < 1) newZoom = 1;
        if (newZoom > 18) newZoom = 18;

        if (newZoom != m_zoomLevel) {
            m_zoomLevel = newZoom;
            update(); // 刷新重绘

            // 可选：在这里打印一下当前的 zoom，方便调试
            // qDebug() << "Current Zoom:" << m_zoomLevel;
        }
    }

    // 接受事件，不再传递给父控件
    event->accept();
}
