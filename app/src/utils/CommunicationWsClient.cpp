#include "CommunicationWsClient.h"
#include "utils/ConfigManager.h"
#include <QDebug>

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

void CommunicationWsClient::initalReqJson() {
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
    QString ip = ConfigManager::instance()->commIp();
    int port = ConfigManager::instance()->commPort();
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
    if (m_pollTimer->isActive()) m_pollTimer->stop();

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

    // 发射信号，Qt 自动处理跨线程传输
    emit sigInternalSendText(jsonReqStateStr);
    // qDebug() << "CommunicationWsClient: 发送指令 ->" << jsonReqStateStr;

    // REQUEST_AGV_TASK
    requestTask["DateTime"] = timeStr;
    requestTask["DataStamps"] = static_cast<int>(dataStamps);

    QJsonDocument docReqTask(requestTask);
    QString jsonReqTaskStr = docReqTask.toJson(QJsonDocument::Compact);

    // 发射信号，Qt 自动处理跨线程传输
    emit sigInternalSendText(jsonReqTaskStr);
    // qDebug() << "CommunicationWsClient: 发送指令 ->" << jsonReqTaskStr;

    dataStamps++;
}

// --- 内部槽函数实现 ---

void CommunicationWsClient::onInternalConnected()
{
    qDebug() << "CommunicationWsClient: 连接建立";
    emit connectionStatusChanged(true);

    // 连接成功后，自动启动定时器
    if (!m_pollTimer->isActive()) {
        m_pollTimer->start();
    }
}

void CommunicationWsClient::onInternalDisconnected()
{
    qDebug() << "CommunicationWsClient: 连接断开";
    emit connectionStatusChanged(false);

    // 连接断开后，自动停止定时器
    // 这样避免了在断网情况下程序还在空转做 JSON 序列化
    m_pollTimer->stop();
}

void CommunicationWsClient::onInternalTextReceived(const QString &msg)
{
    // 处理接收到的消息
    // qDebug() << "CommunicationWsClient 收到业务数据:";
    emit textReceived(msg);
}