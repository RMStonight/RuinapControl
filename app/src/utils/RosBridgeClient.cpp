#include "utils/RosBridgeClient.h"
#include <cmath>
#include <QDateTime>
#include <QtMath>
#include <QJsonArray>

RosBridgeClient::RosBridgeClient(QObject *parent) : QObject(parent), m_webSocket(nullptr)
{
    qRegisterMetaType<QVector<QPointF>>("QVector<QPointF>");
    qRegisterMetaType<QVector<int>>("QVector<int>");

    // 初始化定时器
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    m_reconnectTimer->setInterval(3000); // 设置重连间隔为 3 秒
    connect(m_reconnectTimer, &QTimer::timeout, this, &RosBridgeClient::doReconnect);
}

RosBridgeClient::~RosBridgeClient()
{
    m_needReconnect = false;
    if (m_reconnectTimer)
    {
        m_reconnectTimer->stop();
    }

    if (m_webSocket)
    {
        m_webSocket->abort();
        delete m_webSocket;
    }
}

void RosBridgeClient::connectToRos(const QString &url)
{
    m_url = url;
    m_needReconnect = true; // 允许重连

    // 懒加载：确保 m_webSocket 在 moveToThread 之后的线程中被创建
    if (!m_webSocket)
    {
        m_webSocket = new QWebSocket();
        connect(m_webSocket, &QWebSocket::connected, this, &RosBridgeClient::onConnected);
        connect(m_webSocket, &QWebSocket::disconnected, this, &RosBridgeClient::onSocketDisconnected);
        connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                this, &RosBridgeClient::onSocketError);
        connect(m_webSocket, &QWebSocket::textMessageReceived, this, &RosBridgeClient::onTextMessageReceived);
        connect(m_webSocket, &QWebSocket::binaryMessageReceived, this, &RosBridgeClient::onBinaryMessageReceived);
    }

    // 如果定时器正在运行，先停止，避免冲突
    if (m_reconnectTimer->isActive())
    {
        m_reconnectTimer->stop();
    }

    logger->log(
        QStringLiteral("RosBridgeClient"),
        spdlog::level::info,
        QStringLiteral("Connecting to %1 in thread: %2")
            .arg(url)
            .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()), 0, 16));
    m_webSocket->open(QUrl(url));
}

// 主动关闭连接
void RosBridgeClient::closeConnection()
{
    m_needReconnect = false; // 标记为不需要重连
    if (m_reconnectTimer)
    {
        m_reconnectTimer->stop();
    }
    if (m_webSocket)
    {
        m_webSocket->close();
    }
}

void RosBridgeClient::onConnected()
{
    logger->log(QStringLiteral("RosBridgeClient"), spdlog::level::info, QStringLiteral("Connected!"));
    m_reconnectTimer->stop(); // 连接成功，停止重连定时器
    emit connected();

    // subscribe("/map", "nav_msgs/OccupancyGrid");
    subscribe("/laser_points", "std_msgs/Float32MultiArray");
    subscribe("/map_name", "std_msgs/String");
    subscribe("/agv_state", "std_msgs/Int32MultiArray");
}

// 处理 Socket 断开
void RosBridgeClient::onSocketDisconnected()
{
    logger->log(QStringLiteral("RosBridgeClient"), spdlog::level::err, QStringLiteral("RosBridge: Socket disconnected."));
    emit disconnected(); // 通知 UI

    if (m_needReconnect)
    {
        logger->log(QStringLiteral("RosBridgeClient"), spdlog::level::warn, QStringLiteral("Reconnecting in %1 ms...").arg(m_reconnectTimer->interval()));
        m_reconnectTimer->start();
    }
}

// 处理 Socket 错误
void RosBridgeClient::onSocketError(QAbstractSocket::SocketError error)
{
    logger->log(QStringLiteral("RosBridgeClient"), spdlog::level::err, QStringLiteral("Socket Error: %1 %2").arg(error).arg(m_webSocket->errorString()));

    // 如果是 ConnectionRefused (如 ROS 未启动)，Socket 不会进入 Connected 状态，
    // 可能也不会触发 disconnected 信号，所以这里也需要触发重连逻辑。
    if (m_needReconnect && !m_reconnectTimer->isActive())
    {
        // 稍微延迟重连，避免错误刷屏
        m_reconnectTimer->start();
    }
}

// 执行重连
void RosBridgeClient::doReconnect()
{
    if (m_needReconnect && !m_url.isEmpty())
    {
        logger->log(QStringLiteral("RosBridgeClient"), spdlog::level::warn, QStringLiteral("Attempting to reconnect..."));
        if (m_webSocket)
        {
            m_webSocket->abort(); // 确保处于关闭状态
            m_webSocket->open(QUrl(m_url));
        }
    }
}

// 发布重定位
void RosBridgeClient::setInitialPose(const QPointF &pos, double angle)
{
    if (!m_webSocket || !m_webSocket->isValid())
    {
        logger->log(QStringLiteral("RosBridgeClient"), spdlog::level::err, QStringLiteral("Cannot set pose, socket not connected."));
        return;
    }

    // 1. 计算四元数 (绕Z轴旋转)
    double qz = std::sin(angle / 2.0);
    double qw = std::cos(angle / 2.0);

    // 2. 构造 ROS 消息体 (PoseWithCovarianceStamped 格式)
    QJsonObject msg;

    // Header
    QJsonObject header;
    header["frame_id"] = "map"; // 通常重定位是在 map 坐标系下
    msg["header"] = header;

    // Pose
    QJsonObject pose;
    QJsonObject poseInner;

    // Position
    QJsonObject position;
    position["x"] = pos.x();
    position["y"] = pos.y();
    position["z"] = 0.0;
    poseInner["position"] = position;

    // Orientation (Quaternion)
    QJsonObject orientation;
    orientation["x"] = 0.0;
    orientation["y"] = 0.0;
    orientation["z"] = qz;
    orientation["w"] = qw;
    poseInner["orientation"] = orientation;

    pose["pose"] = poseInner;

    // Covariance (标准重定位通常需要一个协方差矩阵，ROS 默认 36 个 0 即可)
    QJsonArray covariance;
    for (int i = 0; i < 36; ++i)
        covariance.append(0.0);
    pose["covariance"] = covariance;

    msg["pose"] = pose;

    // 3. 构造 ROSBridge 协议外壳
    QJsonObject publishOp;
    publishOp["op"] = "publish";
    publishOp["topic"] = "/baseinipose"; // 对应你要求的 topic
    publishOp["msg"] = msg;

    // 4. 发送
    QJsonDocument doc(publishOp);
    m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));

    logger->log(QStringLiteral("RosBridgeClient"), spdlog::level::warn, QStringLiteral("Sent initial pose x: %1, y: %2, yaw: %3").arg(pos.x()).arg(pos.y()).arg(angle));
}

// 订阅
void RosBridgeClient::subscribe(const QString &topic, const QString &type)
{
    if (m_webSocket)
    {
        QJsonObject json;
        json["op"] = "subscribe";
        json["topic"] = topic;
        json["type"] = type;
        json["compression"] = "cbor"; // 保持 CBOR 压缩
        QJsonDocument doc(json);
        m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
    }
}

void RosBridgeClient::onTextMessageReceived(const QString &message)
{
    // 处理文本消息 (如果有)
}

void RosBridgeClient::onBinaryMessageReceived(const QByteArray &message)
{
    // 直接在当前线程 (子线程) 解析，不涉及跨线程拷贝
    processCborMessage(message);
}

// --- 下面是原 RosDataWorker 的逻辑，合并进来 ---

void RosBridgeClient::processCborMessage(const QByteArray &rawData)
{
    QCborParserError error;
    QCborValue val = QCborValue::fromCbor(rawData, &error);
    if (error.error != QCborError::NoError)
        return;

    if (val.isMap())
    {
        QCborMap map = val.toMap();
        QString op = map[QStringLiteral("op")].toString();
        if (op != "publish")
            return;

        QString topic = map[QStringLiteral("topic")].toString();
        QCborValue msg = map[QStringLiteral("msg")];

        if (topic == "/map")
        {
            // qDebug() << "Parsing Map in Thread:" << QThread::currentThreadId();
            // parseMapCbor(msg);
        }
        else if (topic == "/laser_points")
        {
            parsePointCloudCbor(msg);
        }
        // 处理 map_name
        else if (topic == "/map_name")
        {
            parseMapNameCbor(msg);
        }
        // 处理 agv_state
        else if (topic == "/agv_state")
        {
            parseAgvStateCbor(msg);
        }
    }
}

QByteArray RosBridgeClient::extractByteArray(const QCborValue &val)
{
    if (val.isByteArray())
        return val.toByteArray();
    if (val.isTag())
    {
        QCborValue taggedVal = val.taggedValue();
        if (taggedVal.isByteArray())
            return taggedVal.toByteArray();
    }
    return QByteArray();
}

void RosBridgeClient::parsePointCloudCbor(const QCborValue &msgVal)
{
    QVector<QPointF> points;
    if (!msgVal.isMap())
        return;

    QCborMap msg = msgVal.toMap();
    if (msg.contains(QStringLiteral("data")))
    {
        QCborValue dataVal = msg[QStringLiteral("data")];
        QByteArray byteArray = extractByteArray(dataVal);

        if (!byteArray.isEmpty())
        {
            // Float32 占用 4 字节，每 3 个值为一组 (x, y, 0)
            int floatCount = byteArray.size() / 4;
            const float *raw = reinterpret_cast<const float *>(byteArray.constData());
            for (int i = 0; i + 2 < floatCount; i += 3)
            {
                // 单位为 m
                points.append(QPointF(raw[i], raw[i + 1]));
            }
        }
        else if (dataVal.isArray())
        {
            QCborArray arr = dataVal.toArray();
            for (int i = 0; i + 2 < arr.size(); i += 3)
            {
                points.append(QPointF(arr[i].toDouble(), arr[i + 1].toDouble()));
            }
        }
        emit pointCloudReceived(points);
    }
}

void RosBridgeClient::parseMapNameCbor(const QCborValue &msgVal)
{
    // msgVal 对应 ROS 消息体
    if (msgVal.isMap())
    {
        QCborMap msg = msgVal.toMap();
        // std_msgs/String 只有一个字段: "data"
        if (msg.contains(QStringLiteral("data")))
        {
            QString mapName = msg[QStringLiteral("data")].toString();

            // 发送信号
            emit mapNameReceived(mapName);
        }
    }
}

void RosBridgeClient::parseAgvStateCbor(const QCborValue &msgVal)
{
    QVector<int> agvState;

    if (msgVal.isMap())
    {
        QCborMap msg = msgVal.toMap();

        // 检查是否存在 "data" 字段
        if (msg.contains(QStringLiteral("data")))
        {
            QCborValue dataVal = msg[QStringLiteral("data")];

            // 1. 尝试作为二进制数据解析 (Rosbridge 可能会将 int32[] 压缩为字节流)
            QByteArray byteArray = extractByteArray(dataVal);
            if (!byteArray.isEmpty())
            {
                // Int32 占用 4 字节
                int count = byteArray.size() / 4;
                agvState.reserve(count);

                // 重新解释内存为 int32_t 指针
                const int32_t *raw = reinterpret_cast<const int32_t *>(byteArray.constData());
                for (int i = 0; i < count; ++i)
                {
                    agvState.append(static_cast<int>(raw[i]));
                }
            }
            // 2. 尝试作为普通 CBOR 数组解析 (即标准 JSON 数组格式 [1, 2, 3])
            else if (dataVal.isArray())
            {
                QCborArray arr = dataVal.toArray();
                agvState.reserve(arr.size());
                for (const QCborValue &v : arr)
                {
                    agvState.append(v.toInteger());
                }
            }

            // 仅当解析出数据或确定为空数组时发送信号
            // 这里的条件可以根据你的需求调整，是否允许发送空状态
            emit agvStateReceived(agvState);
        }
    }
}