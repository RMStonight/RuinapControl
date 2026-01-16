#ifndef ROSBRIDGECLIENT_H
#define ROSBRIDGECLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QImage>
#include <QPointF>
#include <QVector>
#include <QThread>
#include "RosDataWorker.h"

class RosBridgeClient : public QObject
{
    Q_OBJECT
public:
    explicit RosBridgeClient(QObject *parent = nullptr);
    ~RosBridgeClient();

    // 连接到 rosbridge server
    void connectToRos(const QString &url);
    // 订阅话题
    void subscribe(const QString &topic, const QString &type);

signals:
    // 连接状态信号
    void connected();
    void disconnected();

    // 数据信号（已解析为 Qt 友好格式）
    // 1. 地图更新 (地图图片, 原点X, 原点Y, 分辨率)
    void mapReceived(const QImage &mapImg, double originX, double originY, double resolution);
    // 2. 雷达更新 (雷达点集, 机器人位置X, 机器人位置Y, 机器人朝向)
    // 注：这里简化处理，实际还需要处理 TF 坐标变换，初期我们可以假设雷达就在 base_link 中心
    void scanReceived(const QVector<QPointF> &points);
    // 增加一个内部信号，用来通知子线程开始工作
    // void startParseMap(const QJsonObject &msg);
    // 给 Worker 的信号，传输原始字符串
    void incomingMessage(QString message);
    void incomingBinaryMessage(QByteArray message); // 新增：用于 CBOR

private slots:
    void onConnected();
    void onTextMessageReceived(const QString &message);
    void onBinaryMessageReceived(const QByteArray &message); // 新增

private:
    QWebSocket m_webSocket;

    // 子线程处理 Map
    QThread m_workerThread;
    RosDataWorker *m_dataWorker;
};

#endif // ROSBRIDGECLIENT_H