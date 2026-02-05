#ifndef NETWORKCHECKTHREAD_H
#define NETWORKCHECKTHREAD_H

#include <QThread>
#include <QObject>
#include <QList>
#include <QMap>
#include <QPair>
#include <QMutex>
#include "LogManager.h"

struct DeviceNode
{
    QString name;
    QString ip;
};

class NetworkCheckThread : public QThread
{
    Q_OBJECT
public:
    explicit NetworkCheckThread(QObject *parent = nullptr);
    ~NetworkCheckThread();

    // 设置主服务器IP
    void setServerIp(const QString &ip);
    // 加载配置文件路径
    void loadConfig(const QString &jsonPath);
    // 停止线程
    void stop();

protected:
    void run() override;

signals:
    // 发送结果信号：text-显示的文字，isNormal-状态是否正常(决定颜色)
    void statusUpdate(QString text, bool isNormal);

private:
    // 日志管理器
    LogManager *logger = &LogManager::instance();

    bool m_stop;
    QMutex m_mutex;
    QString m_serverIp;
    QList<DeviceNode> m_deviceList;
    QMap<QString, QString> m_networkStatusMap; // 用于管理：对象名称 -> 延迟(ms) 或 "离线"

    // 执行一轮后的日志记录逻辑
    void logNetworkSummary();
    // 辅助函数：执行Ping并返回延迟，-1表示不通
    double pingAddress(const QString &ip);
};

#endif // NETWORKCHECKTHREAD_H