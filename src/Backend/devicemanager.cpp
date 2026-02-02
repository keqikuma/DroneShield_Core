#include "devicemanager.h"
#include "../Utils/configloader.h"
#include "Consts.h"
#include <QtMath>

// ============================================================================
// 辅助函数：计算两点经纬度距离 (单位: 米)
// ============================================================================
static double calculateDistance(double lat1, double lng1, double lat2, double lng2)
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

// ============================================================================
// 1. 初始化
// ============================================================================
DeviceManager::DeviceManager(QObject *parent) : QObject(parent)
{
    log("[DeviceManager] 系统核心初始化...");

    // 初始化状态
    m_currentMode = SystemMode::Manual;
    m_isAutoSpoofingRunning = false;
    m_isRelaySuppressionRunning = false;
    m_hasImageThreat = false;

    // 初始化为 0.0
    m_baseLat = 0.0;
    m_baseLng = 0.0;

    // 初始化手动诱骗状态
    m_isManualSpoofing = false;
    m_isInObservationMode = false; // 初始化观察模式状态

    // 防抖定时器 (3秒)
    m_stopDefenseTimer = new QTimer(this);
    m_stopDefenseTimer->setInterval(3000);
    m_stopDefenseTimer->setSingleShot(true);
    connect(m_stopDefenseTimer, &QTimer::timeout, this, &DeviceManager::onStopDefenseTimeout);

    // 【新增】坐标管理器
    m_coordManager = new CoordinateManager(this);

    // 【新增】自动循环定时器 (10秒工作)
    m_autoCycleTimer = new QTimer(this);
    m_autoCycleTimer->setInterval(10000);
    m_autoCycleTimer->setSingleShot(true);
    connect(m_autoCycleTimer, &QTimer::timeout, this, &DeviceManager::onAutoCycleTimeout);

    // 【新增】观察定时器 (2秒观察)
    m_observationTimer = new QTimer(this);
    m_observationTimer->setInterval(2000);
    m_observationTimer->setSingleShot(true);
    connect(m_observationTimer, &QTimer::timeout, this, &DeviceManager::onObservationTimeout);

    // --------------------------------------------------------
    // 读取配置文件
    // --------------------------------------------------------
    ConfigLoader configLoader;
    NetConfig cfgSpoof  = configLoader.getSpoofConfig();
    NetConfig cfgJammer = configLoader.getJammerConfig();
    m_currDetectCfg = configLoader.getDetectConfig();
    m_currRelayCfg  = configLoader.getRelayConfig();
    m_currAmpCfg    = configLoader.getAmpConfig();
    m_currAmp2Cfg   = configLoader.getAmp2Config();

    log(QString("[Config] 模式: %1").arg(
#ifdef SIMULATION_MODE
        "模拟环境"
#else
        "真实硬件"
#endif
        ));

    // --------------------------------------------------------
    // 初始化驱动
    // --------------------------------------------------------

    // 1. 诱骗 (UDP)
    m_spoofDriver = new SpoofDriver(cfgSpoof.ip, cfgSpoof.port, this);
    connect(m_spoofDriver, &SpoofDriver::sigSpoofLog, this, &DeviceManager::sigLogMessage);
    // 连接诱骗坐标
    connect(m_spoofDriver, &SpoofDriver::sigDevicePosition, this, &DeviceManager::onDevicePositionUpdated);

    // 2. 干扰 (HTTP)
    m_jammerDriver = new JammerDriver(this);
    m_jammerDriver->setTarget(cfgJammer.ip, cfgJammer.port);
    connect(m_jammerDriver, &JammerDriver::sigLog, this, &DeviceManager::sigLogMessage);

    // 3. 侦测 (WebSocket)
    m_detectionDriver = new DetectionDriver(this);
    connect(m_detectionDriver, &DetectionDriver::sigDroneListUpdated,
            this, &DeviceManager::onDroneListUpdated);
    connect(m_detectionDriver, &DetectionDriver::sigImageListUpdated,
            this, &DeviceManager::onImageListUpdated);
    connect(m_detectionDriver, &DetectionDriver::sigLog,
            this, &DeviceManager::sigLogMessage);
    connect(m_detectionDriver->findChild<QWebSocket*>(), &QWebSocket::disconnected,
            this, &DeviceManager::onDetectDisconnected);
    QString wsUrl = QString("ws://%1:%2/socket.io/?EIO=3&transport=websocket")
                        .arg(m_currDetectCfg.ip).arg(m_currDetectCfg.port);
    m_detectionDriver->startWork(wsUrl);

    // 4. 压制 (Relay TCP)
    m_relayDriver = new RelayDriver(this);
    connect(m_relayDriver, &RelayDriver::sigLog, this, &DeviceManager::sigLogMessage);
    connect(m_relayDriver, &RelayDriver::sigError, this, &DeviceManager::onRelayError);
    m_relayDriver->connectToDevice(m_currRelayCfg.ip, m_currRelayCfg.port);

    // 5. 功放 (PA) 控制 - 双路 TCP 长连，写频开/关时对两路都发指令
    // ============================================================
    m_ampSocket = new QTcpSocket(this);
    connect(m_ampSocket, &QTcpSocket::errorOccurred, this, &DeviceManager::onAmpError);
    connect(m_ampSocket, &QTcpSocket::connected, this, [this](){
        log("[功放1] TCP 连接成功");
    });
    log(QString("[功放1] 正在连接 %1:%2 ...").arg(m_currAmpCfg.ip).arg(m_currAmpCfg.port));
    m_ampSocket->connectToHost(m_currAmpCfg.ip, m_currAmpCfg.port);

    m_ampSocket2 = new QTcpSocket(this);
    connect(m_ampSocket2, &QTcpSocket::errorOccurred, this, &DeviceManager::onAmp2Error);
    connect(m_ampSocket2, &QTcpSocket::connected, this, [this](){
        log("[功放2] TCP 连接成功");
    });
    log(QString("[功放2] 正在连接 %1:%2 ...").arg(m_currAmp2Cfg.ip).arg(m_currAmp2Cfg.port));
    m_ampSocket2->connectToHost(m_currAmp2Cfg.ip, m_currAmp2Cfg.port);
    // ============================================================

    log("[DeviceManager] 就绪");
}

DeviceManager::~DeviceManager() {}

void DeviceManager::log(const QString &msg) {
    qDebug() << msg;
    emit sigLogMessage(msg);
}

// ============================================================================
// 故障回退逻辑
// ============================================================================
void DeviceManager::onRelayError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    if (!m_relayFallbackUsed &&
        (m_currRelayCfg.ip != Config::DEFAULT_RELAY_IP || m_currRelayCfg.port != Config::DEFAULT_RELAY_PORT))
    {
        log(QString("[警告] 继电器(%1:%2)连接失败，尝试回退到默认配置...").arg(m_currRelayCfg.ip).arg(m_currRelayCfg.port));
        m_relayFallbackUsed = true;
        m_currRelayCfg.ip = Config::DEFAULT_RELAY_IP;
        m_currRelayCfg.port = Config::DEFAULT_RELAY_PORT;
        m_relayDriver->connectToDevice(m_currRelayCfg.ip, m_currRelayCfg.port);
    }
}

void DeviceManager::onDetectDisconnected()
{
    if (!m_detectFallbackUsed &&
        (m_currDetectCfg.ip != Config::DEFAULT_DETECT_IP || m_currDetectCfg.port != Config::DEFAULT_DETECT_PORT))
    {
        log(QString("[警告] 侦测服务(%1:%2)连接断开，尝试回退到默认配置...").arg(m_currDetectCfg.ip).arg(m_currDetectCfg.port));
        m_detectFallbackUsed = true;
        m_currDetectCfg.ip = Config::DEFAULT_DETECT_IP;
        m_currDetectCfg.port = Config::DEFAULT_DETECT_PORT;
        QString wsUrl = QString("ws://%1:%2/socket.io/?EIO=3&transport=websocket")
                            .arg(m_currDetectCfg.ip).arg(m_currDetectCfg.port);
        m_detectionDriver->stopWork();
        m_detectionDriver->startWork(wsUrl);
    }
}

// ============================================================================
// 数据处理 (对接 CoordinateManager)
// ============================================================================

void DeviceManager::onDroneListUpdated(const QList<DroneInfo> &drones)
{
    // 1. 先把原始数据塞给管理器
    m_coordManager->updateDroneList(drones);

    // 2. 再从管理器取"有效"列表 (如果是锁定模式，这里拿到的就是快照)
    QList<DroneInfo> effectiveDrones = m_coordManager->getEffectiveDroneList();

    // 3. 获取当前有效的基站坐标
    double baseLat, baseLng;
    m_coordManager->getEffectiveBasePosition(baseLat, baseLng);

    bool hasDroneThreat = false;
    double minDistance = 999999.0;

    if (!effectiveDrones.isEmpty()) {
        for (auto &d : effectiveDrones) {
            // 计算距离
            if (baseLat > 1.0 && d.uav_lat > 1.0) {
                d.distance = CoordinateManager::calculateDistance(d.uav_lat, d.uav_lng, baseLat, baseLng);
            }

            if (d.whiteList) continue;

            // 只要有目标就视为威胁（放宽条件）
            hasDroneThreat = true;

            // 找最近距离
            if (d.distance <= 0.1) {
                // 如果距离无效，不触发压制，但会触发诱骗
            } else if (d.distance < minDistance) {
                minDistance = d.distance;
            }
        }
    }

    emit sigDroneList(effectiveDrones);
    emit sigTargetsUpdated(effectiveDrones);

    // 如果正在观察模式(暂停中)，我们只更新数据，不触发 processDecision 里的开火逻辑
    if (!m_isInObservationMode) {
        processDecision(hasDroneThreat, minDistance);
    }
}

void DeviceManager::onImageListUpdated(const QList<ImageInfo> &images)
{
    emit sigImageList(images);
    // 只更新标记，不触发决策
    m_hasImageThreat = !images.isEmpty();
}

void DeviceManager::onAlertCountUpdated(int count) { emit sigAlertCount(count); }

// 设备位置更新 (来自 诱骗 SpoofDriver)
void DeviceManager::onDevicePositionUpdated(double lat, double lng)
{
    // 交给管理器去过滤和存储
    m_coordManager->updateBasePosition(lat, lng);

    // 无论是否锁定，都从管理器取"有效"坐标给 UI 显示
    double showLat, showLng;
    m_coordManager->getEffectiveBasePosition(showLat, showLng);

    // 只有非0才更新缓存变量（用于 calculateDistance）
    if (showLat > 1.0) {
        m_baseLat = showLat;
        m_baseLng = showLng;
    }

    emit sigSelfPosition(showLat, showLng);
}

// ============================================================================
// 模式切换与总控
// ============================================================================

void DeviceManager::setSystemMode(SystemMode mode)
{
    if (m_currentMode == mode) return;

    if (mode == SystemMode::Manual) {
        m_stopDefenseTimer->stop();
        m_autoCycleTimer->stop();
        m_observationTimer->stop();
        m_coordManager->unlock(); // 手动模式解锁
    }

    m_currentMode = mode;
    log(QString("[DeviceManager] 切换模式 -> %1").arg(mode == SystemMode::Auto ? "自动" : "手动"));
    stopAllBusiness();
}

void DeviceManager::stopAllBusiness()
{
    m_stopDefenseTimer->stop();
    m_autoCycleTimer->stop();
    m_observationTimer->stop();

    if (m_spoofDriver) m_spoofDriver->setSwitch(false);
    m_isAutoSpoofingRunning = false;
    m_isManualSpoofing = false;
    m_isInObservationMode = false;

    // 务必解锁
    m_coordManager->unlock();

    if (m_jammerDriver) m_jammerDriver->setJamming(false);

    // 【新增】复位时确保功放关闭
    controlAmp(false);

    if (m_relayDriver) m_relayDriver->setAll(false);
    m_isRelaySuppressionRunning = false;

    m_hasImageThreat = false;

    log(">>> 所有设备复位 (OFF)");
}

void DeviceManager::onStopDefenseTimeout()
{
    if (m_currentMode != SystemMode::Auto) return;
    log("[自动决策] 信号丢失超过3秒 -> 停止防御");
    stopAllBusiness();
}

// ============================================================================
// 自动模式决策 (含循环逻辑)
// ============================================================================

void DeviceManager::processDecision(bool hasThreat, double distance)
{
    if (m_currentMode != SystemMode::Auto) return;

    // 目标消失
    if (!hasThreat) {
        if ((m_isAutoSpoofingRunning || m_isRelaySuppressionRunning) && !m_stopDefenseTimer->isActive()) {
            log("[自动决策] 目标消失 -> 启动3秒防抖延时...");
            m_stopDefenseTimer->start();
        }
        return;
    }

    // 目标存在
    if (m_stopDefenseTimer->isActive()) {
        m_stopDefenseTimer->stop();
    }

    // --- 诱骗逻辑 (启动 10秒 循环) ---
    if (!m_isAutoSpoofingRunning) {
        log("[自动决策] 发现威胁 -> 启动诱骗循环");

        // 1. 锁定坐标快照！
        m_coordManager->lock();

        // 2. 获取锁定时的基站坐标用于发射
        double targetLat, targetLng;
        m_coordManager->getEffectiveBasePosition(targetLat, targetLng);

        if (targetLat < 1.0) {
            targetLat = Config::BASE_LAT;
            targetLng = Config::BASE_LON;
            log("[自动决策] 暂未获取基站坐标，使用默认配置");
        } else {
            log(QString("[自动决策] 使用基站实测坐标: %1, %2").arg(targetLat).arg(targetLng));
        }

        m_spoofDriver->setPosition(targetLng, targetLat, 0);
        m_spoofDriver->setSwitch(true);
        m_spoofDriver->startCircular(500, 50);

        m_isAutoSpoofingRunning = true;

        // 3. 启动 10秒 倒计时 (10秒后暂停检查)
        m_autoCycleTimer->start();
    }

    // --- 压制逻辑 (距离 < 10m) ---
    if (distance <= 10.0) {
        if (!m_isRelaySuppressionRunning) {
            log(QString("[自动决策] 进入红区 (%1m) -> 开启压制").arg(distance));
            if (m_relayDriver) m_relayDriver->setAll(true);
            m_isRelaySuppressionRunning = true;
        }
    }
    else {
        if (m_isRelaySuppressionRunning) {
            log("[自动决策] 离开红区 -> 停止压制");
            if (m_relayDriver) m_relayDriver->setAll(false);
            m_isRelaySuppressionRunning = false;
        }
    }
}

// 【新增】10秒工作结束 -> 暂停诱骗
void DeviceManager::onAutoCycleTimeout()
{
    if (m_currentMode != SystemMode::Auto) return;

    log("[自动循环] 10秒工作结束 -> 暂停诱骗，检查环境...");

    // 1. 临时停止诱骗和压制
    m_spoofDriver->setSwitch(false); // 停火
    if (m_relayDriver) m_relayDriver->setAll(false); // 压制也先停一下

    // 2. 解锁坐标 (开始接收实时数据)
    m_coordManager->unlock();

    // 3. 标记状态
    m_isAutoSpoofingRunning = false;
    m_isRelaySuppressionRunning = false;
    m_isInObservationMode = true; // 标记正在观察

    // 4. 启动 2秒 观察定时器
    m_observationTimer->start();
}

// 【新增】2秒观察结束 -> 决定去留
void DeviceManager::onObservationTimeout()
{
    if (m_currentMode != SystemMode::Auto) return;

    m_isInObservationMode = false; // 结束观察

    // 检查刚才 2秒内收到的最新数据
    QList<DroneInfo> liveDrones = m_coordManager->getEffectiveDroneList(); // 此时拿到的是实时的

    bool threatStillExists = false;
    for(auto d : liveDrones) {
        if(!d.whiteList) { threatStillExists = true; break; }
    }

    if (threatStillExists) {
        log("[自动循环] 威胁仍存在 -> 重新启动防御");
        // 立即触发一次刷新
        onDroneListUpdated(liveDrones);
    } else {
        log("[自动循环] 威胁已消除 -> 停止防御");
        stopAllBusiness();
    }
}

// ============================================================================
// 手动模式代码
// ============================================================================
void DeviceManager::setManualSpoofSwitch(bool enable)
{
    m_isManualSpoofing = enable;
    if (enable) m_coordManager->lock();
    else m_coordManager->unlock();

    if(m_spoofDriver) m_spoofDriver->setSwitch(enable);
}

void DeviceManager::setManualCircular()
{
    m_isManualSpoofing = true;
    m_coordManager->lock(); // 手动开启也锁定

    m_spoofDriver->setPosition(Config::BASE_LON, Config::BASE_LAT, 0);
    m_spoofDriver->setSwitch(true);
    m_spoofDriver->startCircular(100, 50);
}

void DeviceManager::setManualDirection(SpoofDirection dir)
{
    m_isManualSpoofing = true;
    m_coordManager->lock();

    m_spoofDriver->setPosition(Config::BASE_LON, Config::BASE_LAT, 0);
    m_spoofDriver->setSwitch(true);
    m_spoofDriver->startDirectional(dir, 15.0);
}

// ============================================================================
// 【新增】功放错误处理 (可选：加回退逻辑，这里暂只打印)
// ============================================================================
void DeviceManager::onAmpError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    log(QString("[功放1] 连接错误: %1").arg(m_ampSocket->errorString()));
}

void DeviceManager::onAmp2Error(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    log(QString("[功放2] 连接错误: %1").arg(m_ampSocket2->errorString()));
}

// ============================================================================
// 【新增】功放控制核心函数：对两路功放都发开/关指令（仅手动干扰开/关与整机复位时调用）
// ============================================================================
void DeviceManager::controlAmp(bool open)
{
    QByteArray cmd = open ? QByteArray::fromHex("AA0101BB") : QByteArray::fromHex("AA0100BB");
    if (open) {
        log("[指令] 功放开启 (AA 01 01 BB) -> 两路");
    } else {
        log("[指令] 功放关闭 (AA 01 00 BB) -> 两路");
    }

    auto sendToSocket = [this, &cmd](QTcpSocket *socket, const NetConfig &cfg, const QString &label) {
        if (socket->state() != QAbstractSocket::ConnectedState) {
            socket->connectToHost(cfg.ip, cfg.port);
            log(QString("[警告] %1 未连接，指令未发送 (已尝试重连)").arg(label));
            return;
        }
        socket->write(cmd);
        socket->flush();
    };

    sendToSocket(m_ampSocket,  m_currAmpCfg,  "功放1");
    sendToSocket(m_ampSocket2, m_currAmp2Cfg, "功放2");
}

void DeviceManager::setJammerConfig(const QList<JammerConfigData> &configs) { if(m_jammerDriver) m_jammerDriver->setWriteFreq(configs); }
void DeviceManager::setManualJammer(bool enable) {
    controlAmp(enable);
    if(m_jammerDriver) m_jammerDriver->setJamming(enable);
}
void DeviceManager::setRelayChannel(int channel, bool on) { if(m_relayDriver) m_relayDriver->setChannel(channel, on); }
void DeviceManager::setRelayAll(bool on) { if(m_relayDriver) m_relayDriver->setAll(on); }
