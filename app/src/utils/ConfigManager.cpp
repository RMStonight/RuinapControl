#include "utils/ConfigManager.h"
#include <QSettings>
#include <QDebug>

// 全局静态变量
static ConfigManager *g_instance = nullptr;

ConfigManager *ConfigManager::instance()
{
    static ConfigManager instance;
    return &instance;
}

ConfigManager::ConfigManager(QObject *parent) : QObject(parent)
{
    // 特殊参数初始化
    m_currentUserRole.store(static_cast<int>(UserRole::Operator));
}

void ConfigManager::load()
{
    QSettings settings; // 自动使用 main.cpp 中定义的 OrganizationName

    // 车体参数
    m_agvId = settings.value("AGV/ID", "1").toString();
    m_agvIp = settings.value("AGV/IP", "127.0.0.1").toString();
    m_chargingThreshold = settings.value("AGV/ChargingThreshold", 100).toInt();
    m_vehicleType = settings.value("AGV/VehicleType", 1).toInt();
    m_mapResolution = settings.value("AGV/MapResolution", 20).toInt();
    m_arcVw = settings.value("AGV/ArcVw", 1000).toInt();
    m_spinVw = settings.value("AGV/SpinVw", 1000).toInt();
    // 文件夹路径
    m_resourceFolder = settings.value("Folder/resource", "").toString();
    m_mapPngFolder = settings.value("Folder/mapPng", "").toString();
    m_mapJsonFolder = settings.value("Folder/mapJson", "").toString();
    m_configFolder = settings.value("Folder/config", "").toString();
    // 网络通讯
    m_commIp = settings.value("Network/CommIp", "host.docker.internal").toString();
    m_commPort = settings.value("Network/CommPort", 9001).toInt();
    m_rosbridgeIp = settings.value("Network/RosBridgeIp", "host.docker.internal").toString();
    m_rosbridgePort = settings.value("Network/RosBridgePort", 9090).toInt();
    m_serverIp = settings.value("Network/ServerIP", "192.168.1.1").toString();
    m_serverPort = settings.value("Network/ServerPort", 8080).toInt();
    // 系统选项
    m_debugMode = settings.value("System/DebugMode", false).toBool();
    m_fullScreen = settings.value("System/FullScreen", false).toBool();

    qDebug() << "ConfigManager: 配置已加载, AGV ID =" << m_agvId;
}

void ConfigManager::save()
{
    // 写操作使用 QWriteLocker，这会阻塞其他读和写
    QReadLocker locker(&m_lock);

    QSettings settings;

    // 车体参数
    settings.setValue("AGV/ID", m_agvId);
    settings.setValue("AGV/IP", m_agvIp);
    settings.setValue("AGV/ChargingThreshold", m_chargingThreshold.load());
    settings.setValue("AGV/VehicleType", m_vehicleType.load());
    settings.setValue("AGV/MapResolution", m_mapResolution.load());
    settings.setValue("AGV/ArcVw", m_arcVw.load());
    settings.setValue("AGV/SpinVw", m_spinVw.load());
    // 文件夹路径
    settings.setValue("Folder/resource", m_resourceFolder);
    settings.setValue("Folder/mapPng", m_mapPngFolder);
    settings.setValue("Folder/mapJson", m_mapJsonFolder);
    settings.setValue("Folder/config", m_configFolder);
    // 网络通讯
    settings.setValue("Network/CommIp", m_commIp);
    settings.setValue("Network/CommPort", m_commPort.load());
    settings.setValue("Network/RosBridgeIp", m_rosbridgeIp);
    settings.setValue("Network/RosBridgePort", m_rosbridgePort.load());
    settings.setValue("Network/ServerIP", m_serverIp);
    settings.setValue("Network/ServerPort", m_serverPort.load());
    // 系统选项
    settings.setValue("System/DebugMode", m_debugMode.load());
    settings.setValue("System/FullScreen", m_fullScreen.load());

    settings.sync(); // 强制写入磁盘

    // 发送信号，通知所有界面更新
    emit configChanged();
    qDebug() << "ConfigManager: 配置已保存并通知所有组件";
}

// --- Getters 实现 ---
// 车体参数
QString ConfigManager::agvId() const
{
    QReadLocker locker(&m_lock);
    return m_agvId;
}
QString ConfigManager::agvIp() const
{
    QReadLocker locker(&m_lock);
    return m_agvIp;
}
int ConfigManager::chargingThreshold() const
{
    return m_chargingThreshold.load();
}
int ConfigManager::vehicleType() const
{
    return m_vehicleType.load();
}
int ConfigManager::mapResolution() const
{
    return m_mapResolution.load();
}
int ConfigManager::arcVw() const
{
    return m_arcVw.load();
}
int ConfigManager::spinVw() const
{
    return m_spinVw.load();
}
// 文件夹路径
QString ConfigManager::resourceFolder() const
{
    QReadLocker locker(&m_lock);
    return m_resourceFolder;
}
QString ConfigManager::mapPngFolder() const
{
    QReadLocker locker(&m_lock);
    return m_mapPngFolder;
}
QString ConfigManager::mapJsonFolder() const
{
    QReadLocker locker(&m_lock);
    return m_mapJsonFolder;
}
QString ConfigManager::configFolder() const
{
    QReadLocker locker(&m_lock);
    return m_configFolder;
}
// 网络通讯
QString ConfigManager::commIp() const
{
    QReadLocker locker(&m_lock);
    return m_commIp;
}
int ConfigManager::commPort() const
{
    return m_commPort.load();
}
QString ConfigManager::rosBridgeIp() const
{
    QReadLocker locker(&m_lock);
    return m_rosbridgeIp;
}
int ConfigManager::rosBridgePort() const
{
    return m_rosbridgePort.load();
}
QString ConfigManager::serverIp() const
{
    QReadLocker locker(&m_lock);
    return m_serverIp;
}
int ConfigManager::serverPort() const
{
    return m_serverPort.load();
}
// 系统选项
UserRole ConfigManager::currentUserRole() const
{
    return static_cast<UserRole>(m_currentUserRole.load());
}
bool ConfigManager::debugMode() const
{
    return m_debugMode.load();
}
bool ConfigManager::fullScreen() const
{
    return m_fullScreen.load();
}

// --- Setters 实现 ---
// 车体参数
void ConfigManager::setAgvId(const QString &id)
{
    QWriteLocker locker(&m_lock);
    m_agvId = id;
}
void ConfigManager::setAgvIp(const QString &ip)
{
    QWriteLocker locker(&m_lock);
    m_agvIp = ip;
}
void ConfigManager::setChargingThreshold(int val)
{
    m_chargingThreshold.store(val);
}
void ConfigManager::setVehicleType(int type)
{
    m_vehicleType.store(type);
}
void ConfigManager::setMapResolution(int res)
{
    m_mapResolution.store(res);
}
void ConfigManager::setArcVw(int vw)
{
    m_arcVw.store(vw);
}
void ConfigManager::setSpinVw(int vw)
{
    m_spinVw.store(vw);
}
// 文件夹路径
void ConfigManager::setResourceFolder(const QString &folder)
{
    QWriteLocker locker(&m_lock);
    m_resourceFolder = folder;
}
void ConfigManager::setMapPngFolder(const QString &folder)
{
    QWriteLocker locker(&m_lock);
    m_mapPngFolder = folder;
}
void ConfigManager::setMapJsonFolder(const QString &folder)
{
    QWriteLocker locker(&m_lock);
    m_mapJsonFolder = folder;
}
void ConfigManager::setConfigFolder(const QString &folder)
{
    QWriteLocker locker(&m_lock);
    m_configFolder = folder;
}
// 网络通讯
void ConfigManager::setCommIp(const QString &ip)
{
    QWriteLocker locker(&m_lock);
    m_commIp = ip;
}
void ConfigManager::setCommPort(int port)
{
    m_commPort.store(port);
}
void ConfigManager::setRosBridgeIp(const QString &ip)
{
    QWriteLocker locker(&m_lock);
    m_rosbridgeIp = ip;
}
void ConfigManager::setRosBridgePort(int port)
{
    m_rosbridgePort.store(port);
}
void ConfigManager::setServerIp(const QString &ip)
{
    QWriteLocker locker(&m_lock);
    m_serverIp = ip;
}
void ConfigManager::setServerPort(int port)
{
    m_serverPort.store(port);
}
// 系统选项
void ConfigManager::setCurrentUserRole(UserRole role)
{
    m_currentUserRole.store(static_cast<int>(role));
}
void ConfigManager::setDebugMode(bool enable)
{
    m_debugMode.store(enable);
}
void ConfigManager::setFullScreen(bool enable)
{
    m_fullScreen.store(enable);
}