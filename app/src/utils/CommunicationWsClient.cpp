#include "CommunicationWsClient.h"

CommunicationWsClient::CommunicationWsClient(QObject *parent)
    : QObject(parent), m_client(nullptr), m_thread(nullptr)
{
    // 初始化数据戳
    dataStamps = 0;

    // 初始化请求 json
    initalReqJson();

    // 初始化定时器
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(POLL_INTERVAL_MS);
    connect(m_pollTimer, &QTimer::timeout, this, &CommunicationWsClient::sendAgvStateRequest);
}

CommunicationWsClient::~CommunicationWsClient()
{
    stop();
}

void CommunicationWsClient::initalReqJson()
{
    // 当前时间戳
    QDateTime cur_time = QDateTime::currentDateTime();
    QString timeStr = cur_time.toString("yyyy-MM-dd hh:mm:ss.zzz");

    requestState["IsSucceed"] = true;
    requestState["DateTime"] = timeStr;
    requestState["DataStamps"] = static_cast<int>(dataStamps);
    requestState["Event"] = "REQUEST_AGV_STATE";
    requestState["Body"] = "";
    requestState["ErrorMessage"] = "";

    requestTask["IsSucceed"] = true;
    requestTask["DateTime"] = timeStr;
    requestTask["DataStamps"] = static_cast<int>(dataStamps);
    requestTask["Event"] = "REQUEST_AGV_TASK";
    requestTask["Body"] = "";
    requestTask["ErrorMessage"] = "";

    touchState["IsSucceed"] = true;
    touchState["DateTime"] = timeStr;
    touchState["DataStamps"] = static_cast<int>(dataStamps);
    touchState["Event"] = "TOUCH_STATE";
    touchState["Body"] = "";
    touchState["ErrorMessage"] = "";
}

void CommunicationWsClient::start()
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
    QString ip = cfg->commIp();
    int port = cfg->commPort();
    QString url = QStringLiteral("ws://%1:%2").arg(ip).arg(port);

    // 4. 绑定信号槽
    // 4.1 线程启动 -> 执行连接
    connect(m_thread, &QThread::started, m_client, [this, url]()
            { m_client->connectToServer(url); });

    // 4.2 线程结束 -> 销毁对象
    connect(m_thread, &QThread::finished, m_client, &QObject::deleteLater);

    // 4.3 底层状态 -> 本类内部槽 -> 转发给外部
    connect(m_client, &WebsocketClient::connected, this, &CommunicationWsClient::onInternalConnected);
    connect(m_client, &WebsocketClient::disconnected, this, &CommunicationWsClient::onInternalDisconnected);
    connect(m_client, &WebsocketClient::textMessageReceived, this, &CommunicationWsClient::onInternalTextReceived);

    // 4.4 发送数据：本类信号(主线程) -> 底层槽(子线程)
    connect(this, &CommunicationWsClient::sigInternalSendText, m_client, &WebsocketClient::sendTextMessage);

    // 5. 启动线程
    m_thread->start();
}

void CommunicationWsClient::stop()
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

void CommunicationWsClient::sendAgvStateRequest()
{
    // 当前时间戳
    QDateTime cur_time = QDateTime::currentDateTime();
    QString timeStr = cur_time.toString("yyyy-MM-dd hh:mm:ss.zzz");

    // REQUEST_AGV_STATE
    requestState["DateTime"] = timeStr;
    requestState["DataStamps"] = static_cast<int>(dataStamps);

    QJsonDocument docReqState(requestState);
    QString jsonReqStateStr = docReqState.toJson(QJsonDocument::Compact);

    emit sigInternalSendText(jsonReqStateStr);

    // REQUEST_AGV_TASK
    requestTask["DateTime"] = timeStr;
    requestTask["DataStamps"] = static_cast<int>(dataStamps);

    QJsonDocument docReqTask(requestTask);
    QString jsonReqTaskStr = docReqTask.toJson(QJsonDocument::Compact);

    emit sigInternalSendText(jsonReqTaskStr);

    // TOUCH_STATE
    touchState["DateTime"] = timeStr;
    touchState["DataStamps"] = static_cast<int>(dataStamps);
    touchState["Body"] = getTouchStateBody();

    QJsonDocument docTouchState(touchState);
    QString jsonTouchStateStr = docTouchState.toJson(QJsonDocument::Compact);

    emit sigInternalSendText(jsonTouchStateStr);

    dataStamps++;
}

QJsonObject CommunicationWsClient::getTouchStateBody()
{
    QJsonObject body;

    // 用于判断是否需要打印，只有移动从 0 变为其他方向才打印一次
    static int lastDir = 0;

    body.insert("page_control", agvData->pageControl());
    body.insert("task_cancel", agvData->taskCancel());
    body.insert("task_start", agvData->taskStart());
    body.insert("task_pause", agvData->taskPause());
    body.insert("task_resume", agvData->taskResume());
    body.insert("charge_cmd", agvData->chargeCmd());
    body.insert("manual_dir", agvData->manualDir());
    body.insert("manual_act", agvData->manualAct());
    body.insert("manual_vy", agvData->manualVy());
    body.insert("ini_x", agvData->iniX());
    body.insert("ini_y", agvData->iniY());
    body.insert("ini_w", agvData->iniW());
    body.insert("music", agvData->music());
    // 根据 manualDir 额外处理 manual_vx 和 manual_vth
    if (agvData->manualDir() == 1 || agvData->manualDir() == 2)
    {
        // 前进、后退
        body.insert("manual_vx", agvData->manualVx());
        body.insert("manual_vth", 0);
    }
    else if (agvData->manualDir() == 5 || agvData->manualDir() == 6 || agvData->manualDir() == 7 || agvData->manualDir() == 8)
    {
        // 左前、右前、左后、右后
        body.insert("manual_vx", agvData->manualVx());
        body.insert("manual_vth", cfg->arcVw());
    }
    else if (agvData->manualDir() == 3 || agvData->manualDir() == 4)
    {
        // 逆自、顺自
        body.insert("manual_vx", agvData->manualVx());
        body.insert("manual_vth", cfg->spinVw());
    }
    else
    {
        // 静止
        body.insert("manual_vx", 0);
        body.insert("manual_vth", 0);
    }

    // 仅在 page_control 开启且按下移动按键时才打印
    if (agvData->pageControl() && agvData->manualDir() != 0 && agvData->manualDir() != lastDir)
    {
        QJsonDocument docBody(body);
        QString bodyStr = docBody.toJson(QJsonDocument::Compact);
        logger->log(QStringLiteral("CommunicationWsClient"), spdlog::level::info, QStringLiteral("TOUCH_STATE Body: %1").arg(bodyStr));
    }

    lastDir = agvData->manualDir();
    return body;
}

// --- 内部槽函数实现 ---

void CommunicationWsClient::onInternalConnected()
{
    logger->log(QStringLiteral("CommunicationWsClient"), spdlog::level::info, QStringLiteral("连接建立"));
    emit connectionStatusChanged(true);

    // 连接成功后，自动启动定时器
    if (!m_pollTimer->isActive())
    {
        m_pollTimer->start();
    }
}

void CommunicationWsClient::onInternalDisconnected()
{
    logger->log(QStringLiteral("CommunicationWsClient"), spdlog::level::err, QStringLiteral("连接断开"));
    emit connectionStatusChanged(false);

    // 连接断开后，自动停止定时器
    // 这样避免了在断网情况下程序还在空转做 JSON 序列化
    m_pollTimer->stop();
}

void CommunicationWsClient::onInternalTextReceived(const QString &msg)
{
    // 处理接收到的消息
    emit textReceived(msg);
}