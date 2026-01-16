#include "utils/RosBridgeClient.h"
#include <QDebug>
#include <cmath>

RosBridgeClient::RosBridgeClient(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<QVector<QPointF>>("QVector<QPointF>");

    connect(&m_webSocket, &QWebSocket::connected, this, &RosBridgeClient::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &RosBridgeClient::disconnected);

    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &RosBridgeClient::onTextMessageReceived);
    connect(&m_webSocket, &QWebSocket::binaryMessageReceived, this, &RosBridgeClient::onBinaryMessageReceived);

    // --- 初始化 Worker 线程 ---
    m_dataWorker = new RosDataWorker(); // 不能指定 parent，因为它要移动到新线程
    m_dataWorker->moveToThread(&m_workerThread);
    // 关键改变：主线程收到字符串 -> 直接丢给 Worker -> Worker 解析 -> Worker 发回结果
    connect(this, &RosBridgeClient::incomingMessage, m_dataWorker, &RosDataWorker::processRawMessage);
    connect(this, &RosBridgeClient::incomingBinaryMessage, m_dataWorker, &RosDataWorker::processCborMessage); // 新增连接

    // Worker 解析完后，通过信号把数据传回 RosBridgeClient (从而传回主线程 UI)
    connect(m_dataWorker, &RosDataWorker::mapParsed, this, &RosBridgeClient::mapReceived);
    connect(m_dataWorker, &RosDataWorker::scanParsed, this, &RosBridgeClient::scanReceived);

    // 启动线程
    m_workerThread.start();
}

// 记得在析构函数中清理线程
RosBridgeClient::~RosBridgeClient()
{
    m_workerThread.quit();
    m_workerThread.wait();
    delete m_dataWorker;
}

// 尝试连接 Ros
void RosBridgeClient::connectToRos(const QString &url)
{
    m_webSocket.open(QUrl(url));
    qDebug() << "RosBridge: Connecting to" << url;
}

// Ros 连接成功
void RosBridgeClient::onConnected()
{
    qDebug() << "RosBridge: Connected!";
    emit connected();

    // 连接成功后，自动订阅常用话题
    // 注意：这里的 topic 名称需要根据你实际的 ROS 设置来（比如 /map, /scan）
    subscribe("/map", "nav_msgs/OccupancyGrid");
    subscribe("/scan", "sensor_msgs/LaserScan");
}

// 订阅 Ros 话题
void RosBridgeClient::subscribe(const QString &topic, const QString &type)
{
    // 构建 rosbridge 订阅协议的 JSON
    QJsonObject json;
    json["op"] = "subscribe";
    json["topic"] = topic;
    json["type"] = type;

    json["compression"] = "cbor";

    QJsonDocument doc(json);
    m_webSocket.sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

// 处理接收的消息
void RosBridgeClient::onTextMessageReceived(const QString &message)
{
    emit incomingMessage(message);
}

void RosBridgeClient::onBinaryMessageReceived(const QByteArray &message)
{
    // 地图等压缩数据走这里
    emit incomingBinaryMessage(message);
}