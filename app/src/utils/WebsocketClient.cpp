#include "WebsocketClient.h"
#include <QDebug>

WebsocketClient::WebsocketClient(QObject *parent) 
    : QObject(parent), m_webSocket(nullptr), m_needReconnect(true)
{
    // 初始化重连定时器
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    m_reconnectTimer->setInterval(3000); // 间隔 3 秒重连
    connect(m_reconnectTimer, &QTimer::timeout, this, &WebsocketClient::doReconnect);
}

WebsocketClient::~WebsocketClient()
{
    // 析构时彻底停止重连并关闭
    m_needReconnect = false;
    if (m_reconnectTimer)
        m_reconnectTimer->stop();

    if (m_webSocket)
    {
        m_webSocket->abort();
        delete m_webSocket;
    }
}

void WebsocketClient::connectToServer(const QString &url)
{
    m_url = url;
    m_needReconnect = true; // 开启重连机制

    // 懒加载：确保 Socket 在当前线程（可能是子线程）中创建
    if (!m_webSocket)
    {
        m_webSocket = new QWebSocket();
        
        connect(m_webSocket, &QWebSocket::connected, this, &WebsocketClient::onConnected);
        connect(m_webSocket, &QWebSocket::disconnected, this, &WebsocketClient::onSocketDisconnected);
        
        // 兼容 Qt 5.15+ 的错误信号写法
        connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                this, &WebsocketClient::onSocketError);
        
        connect(m_webSocket, &QWebSocket::textMessageReceived, this, &WebsocketClient::onTextMessageReceived);
        connect(m_webSocket, &QWebSocket::binaryMessageReceived, this, &WebsocketClient::onBinaryMessageReceived);
    }

    // 如果之前正在等待重连，先停止定时器，立即尝试连接
    if (m_reconnectTimer->isActive()) 
        m_reconnectTimer->stop();

    qDebug() << "WebsocketClient: Connecting to" << url << "in thread:" << QThread::currentThreadId();
    m_webSocket->open(QUrl(url));
}

void WebsocketClient::closeConnection()
{
    m_needReconnect = false; // 用户主动断开，禁止自动重连
    if (m_reconnectTimer) 
        m_reconnectTimer->stop();
    
    if (m_webSocket) 
        m_webSocket->close();
}

void WebsocketClient::sendTextMessage(const QString &message)
{
    if (m_webSocket && m_webSocket->isValid())
    {
        m_webSocket->sendTextMessage(message);
    }
    else
    {
        qWarning() << "WebsocketClient: Cannot send message, socket not connected.";
    }
}

void WebsocketClient::sendBinaryMessage(const QByteArray &data)
{
    if (m_webSocket && m_webSocket->isValid())
    {
        m_webSocket->sendBinaryMessage(data);
    }
    else
    {
        qWarning() << "WebsocketClient: Cannot send binary data, socket not connected.";
    }
}

void WebsocketClient::onConnected()
{
    qDebug() << "WebsocketClient: Connected!";
    m_reconnectTimer->stop(); // 连接成功，停止重连倒计时
    emit connected();
}

void WebsocketClient::onSocketDisconnected()
{
    qDebug() << "WebsocketClient: Disconnected.";
    emit disconnected();

    // 只有在非主动关闭的情况下才重连
    if (m_needReconnect)
    {
        qDebug() << "WebsocketClient: Reconnecting in" << m_reconnectTimer->interval() << "ms...";
        m_reconnectTimer->start();
    }
}

void WebsocketClient::onSocketError(QAbstractSocket::SocketError error)
{
    QString errStr = m_webSocket->errorString();
    qDebug() << "WebsocketClient: Error:" << error << errStr;
    emit errorOccurred(errStr);

    // 如果连接被拒绝（服务没起）或其他网络错误，也触发重连机制
    if (m_needReconnect && !m_reconnectTimer->isActive())
    {
        m_reconnectTimer->start();
    }
}

void WebsocketClient::doReconnect()
{
    if (m_needReconnect && !m_url.isEmpty())
    {
        qDebug() << "WebsocketClient: Attempting to reconnect...";
        if (m_webSocket) 
        {
             m_webSocket->abort(); // 确保复位状态
             m_webSocket->open(QUrl(m_url));
        }
    }
}

void WebsocketClient::onTextMessageReceived(const QString &message)
{
    // 纯粹的数据透传，不做业务解析
    emit textMessageReceived(message);
}

void WebsocketClient::onBinaryMessageReceived(const QByteArray &message)
{
    // 纯粹的数据透传
    emit binaryMessageReceived(message);
}