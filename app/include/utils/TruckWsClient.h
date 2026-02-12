#ifndef TRUCKWSCLIENT_H
#define TRUCKWSCLIENT_H

#include <QObject>
#include <QThread>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include "WebsocketClient.h" // 引用之前的底层客户端
#include "utils/ConfigManager.h"
#include "AgvData.h"

class TruckWsClient : public QObject
{
    Q_OBJECT
public:
    explicit TruckWsClient(QObject *parent = nullptr);
    ~TruckWsClient();

    // 启动通信（读取配置、开启线程、连接）
    void start();
    // 停止通信
    void stop();

    // --- 业务接口 ---
    // 发送状态请求 (封装具体的 JSON 协议)
    void sendRequest();
    void requestTruckSize();

signals:
    // --- 向外（UI）暴露的信号 ---
    // 连接状态改变：true=在线，false=离线
    void connectionStatusChanged(bool isConnected);
    // --- 内部信号 (用于跨线程通讯) ---
    void sigInternalSendText(const QString &msg);
    // 获得 TRUCK_SIZE 的信号
    void getTruckSize(const QString &dateTime, int width, int depth);

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

    // 日志管理器
    LogManager *logger = &LogManager::instance();
    
    QTimer *m_pollTimer;             // 用于持续触发请求
    const int POLL_INTERVAL_MS = 50; // 轮询间隔
    uint64_t dataStamps;             // 数据戳

    // QJsonObject requestState; // 轮询 AGV_STATE

    void initalReqJson(); // 初始化请求 json

    ConfigManager *cfg = ConfigManager::instance(); // cfg
    AgvData *agvData = AgvData::instance();         // agvData

    // 解析接收到的数据
    void parseMsg(const QString &msg);
    bool tryParseJson(const QString &jsonStr, QJsonObject &resultObj); 

    void parseTruckSize(const QJsonObject &root);   // 解析 TRUCK_SIZE
};

#endif // TRUCKWSCLIENT_H