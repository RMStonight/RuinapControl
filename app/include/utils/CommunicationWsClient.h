#ifndef COMMUNICATIONWSCLIENT_H
#define COMMUNICATIONWSCLIENT_H

#include <QObject>
#include <QThread>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include "utils/WebsocketClient.h" // 引用之前的底层客户端

class CommunicationWsClient : public QObject
{
    Q_OBJECT
public:
    explicit CommunicationWsClient(QObject *parent = nullptr);
    ~CommunicationWsClient();

    // 启动通信（读取配置、开启线程、连接）
    void start();
    // 停止通信
    void stop();

    // --- 业务接口 ---
    // 发送 AGV 状态请求 (封装具体的 JSON 协议)
    void sendAgvStateRequest();

signals:
    // --- 向外（UI）暴露的信号 ---
    // 连接状态改变：true=在线，false=离线
    void connectionStatusChanged(bool isConnected);
    void textReceived(const QString &msg);

signals:
    // --- 内部信号 (用于跨线程通讯) ---
    void sigInternalSendText(const QString &msg);

private slots:
    // 内部处理底层连接成功
    void onInternalConnected();
    // 内部处理底层断开
    void onInternalDisconnected();
    // 内部处理底层消息
    void onInternalTextReceived(const QString &msg);

private:
    WebsocketClient *m_client;
    QThread *m_thread;

    QTimer *m_pollTimer;                // 用于持续触发请求
    const int POLL_INTERVAL_MS = 50;    // 轮询间隔
    uint64_t dataStamps;                // 数据戳
    QJsonObject requestState;           // 轮询 AGV_STATE
    QJsonObject requestTask;            // 轮询 AGV_TASK

    void initalReqJson();               // 初始化请求 json
};

#endif // COMMUNICATIONWSCLIENT_H