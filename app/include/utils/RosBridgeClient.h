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

class RosBridgeClient : public QObject
{
    Q_OBJECT
public:
    explicit RosBridgeClient(QObject *parent = nullptr);
    ~RosBridgeClient();

public slots:
    // 改为 Slot，因为必须在线程启动后调用
    void connectToRos(const QString &url);
    void subscribe(const QString &topic, const QString &type);

signals:
    void connected();
    void disconnected();
    // 数据信号发送给 UI
    void scanReceived(QVector<QPointF> points);

private slots:
    void onConnected();
    void onTextMessageReceived(const QString &message);
    void onBinaryMessageReceived(const QByteArray &message);

private:
    // 解析函数 (原 Worker 的逻辑)
    void processCborMessage(const QByteArray &rawData);
    // void parseMapCbor(const QCborValue &msg);
    void parseScanCbor(const QCborValue &msg);
    QByteArray extractByteArray(const QCborValue &val);

private:
    QWebSocket *m_webSocket; // 改为指针，确保在正确的线程初始化
};

#endif // ROSBRIDGECLIENT_H