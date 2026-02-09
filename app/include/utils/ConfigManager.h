#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QReadWriteLock>
#include <atomic>
#include "LogManager.h"

// 枚举类型定义
enum class UserRole
{
    Operator = 1, // 普通用户
    Admin = 2,    // 管理员
    Developer = 3 // 开发者
};

class ConfigManager : public QObject
{
    Q_OBJECT

public:
    // 获取单例实例
    static ConfigManager *instance();

    // 特殊参数，不读取 config 配置文件
    UserRole currentUserRole() const;
    void setCurrentUserRole(UserRole role);

    // 初始化/加载配置 (在 main.cpp 调用)
    void load();
    // 保存配置到文件
    void save();

    // --- Getters (供界面读取) ---
    // 车体参数
    QString agvId() const;
    QString agvIp() const;
    int chargingThreshold() const;
    int arcVw() const;
    int spinVw() const;
    int vehicleType() const;
    int mapResolution() const;
    // 文件夹路径
    QString resourceFolder() const;
    QString mapPngFolder() const;
    QString mapJsonFolder() const;
    QString configFolder() const;
    QString logFolder() const;
    // 网络通信
    QString commIp() const;
    int commPort() const;
    QString rosBridgeIp() const;
    int rosBridgePort() const;
    QString serverIp() const;
    int serverPort() const;
    // 其他通讯
    QString microControllerCom() const;
    int microControllerComBaudrate() const;
    // 系统运行
    int adminDuration() const;
    bool debugMode() const;
    bool fullScreen() const;

    // --- Setters (供设置界面修改) ---
    // 车体参数
    void setAgvId(const QString &id);
    void setAgvIp(const QString &ip);
    void setChargingThreshold(int val);
    void setArcVw(int vw);
    void setSpinVw(int vw);
    void setVehicleType(int type);
    void setMapResolution(int res);
    // 文件夹路径
    void setResourceFolder(const QString &folder);
    void setMapPngFolder(const QString &folder);
    void setMapJsonFolder(const QString &folder);
    void setConfigFolder(const QString &folder);
    void setLogFolder(const QString &folder);
    // 网络通信
    void setCommIp(const QString &ip);
    void setCommPort(int port);
    void setRosBridgeIp(const QString &ip);
    void setRosBridgePort(int port);
    void setServerIp(const QString &ip);
    void setServerPort(int port);
    // 其他通讯
    void setMicroControllerCom(const QString &com);
    void setMicroControllerComBaudrate(int baudrate);
    // 系统运行
    void setAdminDuration(int duration);
    void setDebugMode(bool enable);
    void setFullScreen(bool enable);

signals:
    // 当保存配置时触发，所有监听者(如Header)收到此信号后自我刷新
    void configChanged();
    void userRoleChanged(UserRole newRole); // 用户权限变化

private:
    explicit ConfigManager(QObject *parent = nullptr); // 私有构造

    // 日志管理器
    LogManager *logger = &LogManager::instance();

    // 特殊参数，不读取 config 配置文件
    std::atomic<int> m_currentUserRole; // 当前用户，1 operator，2 admin

    // 内存中的变量缓存
    // 车体参数
    QString m_agvId;
    QString m_agvIp;
    std::atomic<int> m_chargingThreshold; // 充电自动取消的阈值
    std::atomic<int> m_arcVw;             // 弧线速度
    std::atomic<int> m_spinVw;            // 原地自旋速度
    std::atomic<int> m_vehicleType;       // 1 = 双叉叉车，2 = 四叉叉车
    std::atomic<int> m_mapResolution;     // 20, 50, 100，地图分辨率，0.02， 0.05， 0.1
    // 文件夹路径
    QString m_resourceFolder;
    QString m_mapPngFolder;
    QString m_mapJsonFolder;
    QString m_configFolder;
    QString m_logFolder;
    // 网络通信
    QString m_commIp;
    std::atomic<int> m_commPort;
    QString m_rosbridgeIp;
    std::atomic<int> m_rosbridgePort;
    QString m_serverIp;
    std::atomic<int> m_serverPort;
    // 其他通讯
    QString m_microControllerCom;                  // 单片机 com 口
    std::atomic<int> m_microControllerComBaudrate; // 单片机 com 口波特率
    // 系统运行
    std::atomic<int> m_adminDuration; // 管理员权限在无任何操作下维持的时长
    std::atomic<bool> m_debugMode;
    std::atomic<bool> m_fullScreen;

    // mutable 允许在 const 函数中加锁
    mutable QReadWriteLock m_lock;
};

#endif // CONFIGMANAGER_H