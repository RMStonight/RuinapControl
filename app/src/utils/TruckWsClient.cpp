#include "TruckWsClient.h"

TruckWsClient::TruckWsClient(QObject *parent)
    : QObject(parent), m_client(nullptr), m_thread(nullptr)
{
    // 初始化数据戳
    dataStamps = 0;

    // 初始化请求 json
    initalReqJson();

    // 初始化定时器
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(POLL_INTERVAL_MS);
    connect(m_pollTimer, &QTimer::timeout, this, &TruckWsClient::sendRequest);
}

TruckWsClient::~TruckWsClient()
{
    stop();
}

void TruckWsClient::initalReqJson()
{
    // 当前时间戳
    QDateTime cur_time = QDateTime::currentDateTime();
    QString timeStr = cur_time.toString("yyyy-MM-dd hh:mm:ss.zzz");

    // requestState["IsSucceed"] = true;
    // requestState["DateTime"] = timeStr;
    // requestState["DataStamps"] = static_cast<int>(dataStamps);
    // requestState["Event"] = "REQUEST_AGV_STATE";
    // requestState["Body"] = "";
    // requestState["ErrorMessage"] = "";
}

void TruckWsClient::start()
{
    if (m_thread && m_thread->isRunning())
    {
        return; // 避免重复启动
    }

    // 1. 创建对象和线程
    m_thread = new QThread(this);
    m_client = new WebsocketClient(); // 注意：不能加 parent，因为要 moveToThread

    // 2. 移动到子线程
    m_client->moveToThread(m_thread);

    // 3. 读取配置
    QString ip = cfg->truckLoadingIp();
    int port = cfg->truckLoadingPort();
    QString url = QStringLiteral("ws://%1:%2").arg(ip).arg(port);

    // 4. 绑定信号槽
    // 4.1 线程启动 -> 执行连接
    connect(m_thread, &QThread::started, m_client, [this, url]()
            { m_client->connectToServer(url); });

    // 4.2 线程结束 -> 销毁对象
    connect(m_thread, &QThread::finished, m_client, &QObject::deleteLater);

    // 4.3 底层状态 -> 本类内部槽 -> 转发给外部
    connect(m_client, &WebsocketClient::connected, this, &TruckWsClient::onInternalConnected);
    connect(m_client, &WebsocketClient::disconnected, this, &TruckWsClient::onInternalDisconnected);
    connect(m_client, &WebsocketClient::textMessageReceived, this, &TruckWsClient::onInternalTextReceived);

    // 4.4 发送数据：本类信号(主线程) -> 底层槽(子线程)
    connect(this, &TruckWsClient::sigInternalSendText, m_client, &WebsocketClient::sendTextMessage);

    // 5. 启动线程
    m_thread->start();
}

void TruckWsClient::stop()
{
    // 停止时必须关闭定时器，防止向已销毁的线程发送信号
    if (m_pollTimer->isActive())
        m_pollTimer->stop();

    if (m_thread && m_thread->isRunning())
    {
        // 请求子线程退出
        m_thread->quit();
        m_thread->wait();
    }
    // m_client 会自动析构，因为绑定了 finished->deleteLater
    // m_thread 是 this 的子对象，也会自动析构，或者可以手动 delete
}

// --- 业务逻辑封装区域 ---

void TruckWsClient::sendRequest()
{
    // 当前时间戳
    QDateTime cur_time = QDateTime::currentDateTime();
    QString timeStr = cur_time.toString("yyyy-MM-dd hh:mm:ss.zzz");

    // REQUEST_AGV_STATE
    // requestState["DateTime"] = timeStr;
    // requestState["DataStamps"] = static_cast<int>(dataStamps);

    // QJsonDocument docReqState(requestState);
    // QString jsonReqStateStr = docReqState.toJson(QJsonDocument::Compact);

    // emit sigInternalSendText(jsonReqStateStr);

    // // REQUEST_AGV_TASK
    // requestTask["DateTime"] = timeStr;
    // requestTask["DataStamps"] = static_cast<int>(dataStamps);

    // QJsonDocument docReqTask(requestTask);
    // QString jsonReqTaskStr = docReqTask.toJson(QJsonDocument::Compact);

    // emit sigInternalSendText(jsonReqTaskStr);

    // QJsonDocument docTouchState(touchState);
    // QString jsonTouchStateStr = docTouchState.toJson(QJsonDocument::Compact);

    // emit sigInternalSendText(jsonTouchStateStr);

    dataStamps++;
}

// --- 内部槽函数实现 ---

void TruckWsClient::onInternalConnected()
{
    logger->log(QStringLiteral("TruckWsClient"), spdlog::level::info, QStringLiteral("连接建立"));
    emit connectionStatusChanged(true);

    // 连接成功后，自动启动定时器
    if (!m_pollTimer->isActive())
    {
        m_pollTimer->start();
    }
}

void TruckWsClient::onInternalDisconnected()
{
    logger->log(QStringLiteral("TruckWsClient"), spdlog::level::err, QStringLiteral("连接断开"));
    emit connectionStatusChanged(false);

    // 连接断开后，自动停止定时器
    // 这样避免了在断网情况下程序还在空转做 JSON 序列化
    m_pollTimer->stop();
}

void TruckWsClient::onInternalTextReceived(const QString &msg)
{
    // logger->log("TruckWsClient", spdlog::level::info, msg);
    // 解析接收到的消息
    parseMsg(msg);
}

void TruckWsClient::requestTruckSize()
{
    QJsonObject req;

    QDateTime cur_time = QDateTime::currentDateTime();
    QString timeStr = cur_time.toString("yyyy-MM-dd hh:mm:ss.zzz");

    req.insert(QStringLiteral("DateTime"), timeStr);
    req.insert(QStringLiteral("Event"), QStringLiteral("REQUEST_TRUCK_SIZE"));

    QJsonDocument docReq(req);
    QString jsonReqeStr = docReq.toJson(QJsonDocument::Compact);

    emit sigInternalSendText(jsonReqeStr);
}

void TruckWsClient::parseMsg(const QString &msg)
{
    QJsonObject root;

    if (!tryParseJson(msg, root))
    {
        logger->log(QStringLiteral("TruckWsClient"), spdlog::level::warn, QStringLiteral("parseMsg 发生错误，非法的 json 结构"));
        return; // 校验失败
    }

    // 提取 Event 类型
    if (!root.value("Event").isString())
    {
        logger->log(QStringLiteral("TruckWsClient"), spdlog::level::warn, QStringLiteral("TruckWsClient 缺少 Event 字段 或者 Event 字段不是 String 类型"));
        return;
    }
    QString event = root.value("Event").toString();

    // 提取 Body 数据实体
    // 注意：Body 可能是 Object，也可能是 Array，这里假设你的业务数据主要是 Object
    if (!root.value("Body").isObject())
    {
        logger->log(QStringLiteral("TruckWsClient"), spdlog::level::warn, QStringLiteral("TruckWsClient 缺少 Body 字段 或者 Body 字段不是 JSON 对象"));
        return;
    }
    QJsonObject body = root.value("Body").toObject();

    // 根据 Event 分发处理
    if (event == "TRUCK_SIZE")
    {
        parseTruckSize(root);
    }
    else
    {
        logger->log(QStringLiteral("TruckWsClient"), spdlog::level::warn, QStringLiteral("忽略未知的事件类型: %1").arg(event));
    }
}

// 解析 JSON 结构
bool TruckWsClient::tryParseJson(const QString &jsonStr, QJsonObject &resultObj)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError)
    {
        QString jsonParseErr = "TruckLoading JSON 解析错误:" + parseError.errorString();
        logger->log(QStringLiteral("TruckWsClient"), spdlog::level::warn, jsonParseErr);
        return false;
    }

    if (!doc.isObject())
    {
        logger->log(QStringLiteral("TruckWsClient"), spdlog::level::warn, QStringLiteral("TruckLoading 数据格式错误: 不是 JSON 对象"));
        return false;
    }

    resultObj = doc.object();
    return true;
}

// 解析 TRUCK_SIZE
void TruckWsClient::parseTruckSize(const QJsonObject &root)
{
    QString dateTime = root.value("DateTime").toString();
    QJsonObject body = root.value("Body").toObject();

    // 解析 AGVInfo
    // 校验字段是否存在
    if (!body.value("Width").isDouble() || !body.value("Depth").isDouble())
    {
        logger->log(QStringLiteral("TruckWsClient"), spdlog::level::warn, QStringLiteral("TRUCK_SIZE 错误: 缺少 Width、Depth 字段或者 Width、Depth 不是数字"));
        return;
    }

    int truckWidth = body.value("Width").toInt();
    int truckDepth = body.value("Depth").toInt();

    emit getTruckSize(dateTime, truckWidth, truckDepth);
    // logger->log(QStringLiteral("TruckWsClient"), spdlog::level::info, QStringLiteral("TRUCK_SIZE DateTime: %1, Width: %2, Depth: %3").arg(dateTime).arg(truckWidth).arg(truckDepth));
}