#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QTimer>
#include <QThread>
#include "LogManager.h"

class WebsocketClient : public QObject
{
    Q_OBJECT
public:
    explicit WebsocketClient(QObject *parent = nullptr);
    ~WebsocketClient();

public slots:
    // 连接到指定 URL
    void connectToServer(const QString &url);
    
    // 主动断开连接
    void closeConnection();
    
    // 发送文本消息
    void sendTextMessage(const QString &message);
    
    // 发送二进制消息
    void sendBinaryMessage(const QByteArray &data);

signals:
    // 状态信号
    void connected();
    void disconnected();
    void errorOccurred(const QString &errorMsg);

    // 数据信号（转发给外部业务逻辑处理）
    void textMessageReceived(const QString &message);
    void binaryMessageReceived(const QByteArray &data);

private slots:
    // Socket 内部信号处理
    void onConnected();
    void onTextMessageReceived(const QString &message);
    void onBinaryMessageReceived(const QByteArray &message);
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    
    // 定时重连槽函数
    void doReconnect();

private:
    // 日志管理器
    LogManager *logger = &LogManager::instance();
    
    QWebSocket *m_webSocket;    // WebSocket 指针
    QTimer *m_reconnectTimer;   // 重连定时器
    QString m_url;              // 保存连接地址
    bool m_needReconnect;       // 重连标志位
};

#endif // WEBSOCKETCLIENT_H