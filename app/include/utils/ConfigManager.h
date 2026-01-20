#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QReadWriteLock>
#include <atomic>

class ConfigManager : public QObject
{
    Q_OBJECT

public:
    // 获取单例实例
    static ConfigManager *instance();

    // 初始化/加载配置 (在 main.cpp 调用)
    void load();
    // 保存配置到文件
    void save();

    // --- Getters (供界面读取) ---
    // 车体参数
    QString agvId() const;
    QString agvIp() const;
    int maxSpeed() const;
    // 文件夹路径
    QString resourceFolder() const;
    QString mapPngFolder() const;
    // 网络通信
    QString commIp() const;
    int commPort() const;
    QString rosBridgeIp() const;
    int rosBridgePort() const;
    QString serverIp() const;
    int serverPort() const;
    // 系统运行
    bool autoConnect() const;
    bool debugMode() const;
    bool fullScreen() const;

    // --- Setters (供设置界面修改) ---
    // 车体参数
    void setAgvId(const QString &id);
    void setAgvIp(const QString &ip);
    void setMaxSpeed(int speed);
    // 文件夹路径
    void setResourceFolder(const QString &folder);
    void setMapPngFolder(const QString &folder);
    // 网络通信
    void setCommIp(const QString &ip);
    void setCommPort(int port);
    void setRosBridgeIp(const QString &ip);
    void setRosBridgePort(int port);
    void setServerIp(const QString &ip);
    void setServerPort(int port);
    // 系统运行
    void setAutoConnect(bool enable);
    void setDebugMode(bool enable);
    void setFullScreen(bool enable);

signals:
    // 当保存配置时触发，所有监听者(如Header)收到此信号后自我刷新
    void configChanged();

private:
    explicit ConfigManager(QObject *parent = nullptr); // 私有构造

    // 内存中的变量缓存
    // 车体参数
    QString m_agvId;
    QString m_agvIp;
    std::atomic<int> m_maxSpeed;
    // 文件夹路径
    QString m_resourceFolder;
    QString m_mapPngFolder;
    // 网络通信
    QString m_commIp;
    std::atomic<int> m_commPort;
    QString m_rosbridgeIp;
    std::atomic<int> m_rosbridgePort;
    QString m_serverIp;
    std::atomic<int> m_serverPort;
    // 系统运行
    std::atomic<bool> m_autoConnect;
    std::atomic<bool> m_debugMode;
    std::atomic<bool> m_fullScreen;
    
    // mutable 允许在 const 函数中加锁
    mutable QReadWriteLock m_lock;
};

#endif // CONFIGMANAGER_H