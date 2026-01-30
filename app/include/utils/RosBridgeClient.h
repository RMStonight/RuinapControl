#ifndef ROSBRIDGECLIENT_H
#define ROSBRIDGECLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QImage>
#include <QVector>
#include <QPointF>
#include <QCborValue>
#include <QCborMap>
#include <QCborArray>
#include <QThread>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>

class RosBridgeClient : public QObject
{
    Q_OBJECT
public:
    explicit RosBridgeClient(QObject *parent = nullptr);
    ~RosBridgeClient();

public slots:
    // 改为 Slot，因为必须在线程启动后调用
    void connectToRos(const QString &url);
    void closeConnection();
    void subscribe(const QString &topic, const QString &type);

signals:
    void connected();
    void disconnected();
    // 数据信号发送给 UI
    void pointCloudReceived(QVector<QPointF> points);
    void mapNameReceived(QString mapName);
    void agvStateReceived(QVector<int> agvState);

private slots:
    void onConnected();
    void onTextMessageReceived(const QString &message);
    void onBinaryMessageReceived(const QByteArray &message);
    // 内部处理断开连接
    void onSocketDisconnected();
    // 处理 Socket 错误
    void onSocketError(QAbstractSocket::SocketError error);
    // 执行重连
    void doReconnect();

private:
    // 解析函数 (原 Worker 的逻辑)
    void processCborMessage(const QByteArray &rawData);
    void parsePointCloudCbor(const QCborValue &msg);
    void parseMapNameCbor(const QCborValue &msg);
    void parseAgvStateCbor(const QCborValue &msg);
    
    QByteArray extractByteArray(const QCborValue &val);

private:
    QWebSocket *m_webSocket;    // 改为指针，确保在正确的线程初始化
    QTimer *m_reconnectTimer;   // 重连定时器
    QString m_url;              // 记录连接地址
    bool m_needReconnect;       // 控制是否自动重连
};

#endif // ROSBRIDGECLIENT_H