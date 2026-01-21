#include "utils/RosBridgeClient.h"
#include <QDebug>
#include <cmath>
#include <QDateTime>

RosBridgeClient::RosBridgeClient(QObject *parent) : QObject(parent), m_webSocket(nullptr)
{
    qRegisterMetaType<QVector<QPointF>>("QVector<QPointF>");

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
    if (m_reconnectTimer->isActive()) m_reconnectTimer->stop();

    qDebug() << "RosBridge: Connecting to" << url << "in thread:" << QThread::currentThreadId();
    m_webSocket->open(QUrl(url));
}

// 主动关闭连接
void RosBridgeClient::closeConnection()
{
    m_needReconnect = false; // 标记为不需要重连
    if (m_reconnectTimer) m_reconnectTimer->stop();
    if (m_webSocket) m_webSocket->close();
}

void RosBridgeClient::onConnected()
{
    qDebug() << "RosBridge: Connected!";
    m_reconnectTimer->stop(); // 连接成功，停止重连定时器
    emit connected();

    // subscribe("/map", "nav_msgs/OccupancyGrid");
    subscribe("/scan", "sensor_msgs/LaserScan");
}

// 处理 Socket 断开
void RosBridgeClient::onSocketDisconnected()
{
    qDebug() << "RosBridge: Socket disconnected.";
    emit disconnected(); // 通知 UI

    if (m_needReconnect)
    {
        qDebug() << "RosBridge: Reconnecting in" << m_reconnectTimer->interval() << "ms...";
        m_reconnectTimer->start();
    }
}

// 处理 Socket 错误
void RosBridgeClient::onSocketError(QAbstractSocket::SocketError error)
{
    qDebug() << "RosBridge: Socket Error:" << error << m_webSocket->errorString();
    
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
        qDebug() << "RosBridge: Attempting to reconnect...";
        if (m_webSocket) {
             m_webSocket->abort(); // 确保处于关闭状态
             m_webSocket->open(QUrl(m_url));
        }
    }
}

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
        else if (topic == "/scan")
        {
            parseScanCbor(msg);
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

void RosBridgeClient::parseScanCbor(const QCborValue &msgVal)
{
    // 保持原逻辑不变...
    QCborMap msg = msgVal.toMap();
    double angle_min = msg[QStringLiteral("angle_min")].toDouble();
    double angle_increment = msg[QStringLiteral("angle_increment")].toDouble();
    double range_min = msg[QStringLiteral("range_min")].toDouble();
    double range_max = msg[QStringLiteral("range_max")].toDouble();

    QCborValue rangesVal = msg[QStringLiteral("ranges")];
    QVector<QPointF> points;
    QByteArray data = extractByteArray(rangesVal);

    if (!data.isEmpty())
    {
        int count = data.size() / 4;
        points.reserve(count);
        const float *rawRanges = reinterpret_cast<const float *>(data.constData());
        for (int i = 0; i < count; ++i)
        {
            float r = rawRanges[i];
            if (std::isfinite(r) && r > range_min && r < range_max)
            {
                double angle = angle_min + i * angle_increment;
                points.append(QPointF(r * cos(angle), r * sin(angle)));
            }
        }
    }
    else if (rangesVal.isArray())
    {
        QCborArray ranges = rangesVal.toArray();
        points.reserve(ranges.size());
        for (int i = 0; i < ranges.size(); ++i)
        {
            double r = ranges[i].toDouble();
            if (r > range_min && r < range_max)
            {
                double angle = angle_min + i * angle_increment;
                points.append(QPointF(r * cos(angle), r * sin(angle)));
            }
        }
    }
    emit scanReceived(points);
}