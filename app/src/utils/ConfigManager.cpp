#include "utils/ConfigManager.h"
#include <QSettings>
#include <QDebug>

// 全局静态变量
static ConfigManager *g_instance = nullptr;

ConfigManager *ConfigManager::instance()
{
    if (!g_instance)
    {
        g_instance = new ConfigManager();
    }
    return g_instance;
}

ConfigManager::ConfigManager(QObject *parent) : QObject(parent)
{
    // 设置默认值，防止第一次运行读不到数据
    // 车体参数
    m_agvId = "1";
    m_agvIp = "127.0.0.1";
    m_maxSpeed = 100;
    // 文件夹路径
    m_resourceFolder = "/home/ruinap/config_qt/resource";
    m_mapPngFolder = "/home/ruinap/map";
    // 网络通讯
    m_commIp = "host.docker.internal";
    m_commPort = 9001;
    m_rosbridgeIp = "host.docker.internal";
    m_rosbridgePort = 9090;
    m_serverIp = "192.168.1.1";
    m_serverPort = 8080;
    // 系统选项
    m_autoConnect = false;
    m_debugMode = false;
    m_fullScreen = false;
}

void ConfigManager::load()
{
    QSettings settings; // 自动使用 main.cpp 中定义的 OrganizationName

    // 车体参数
    m_agvId = settings.value("AGV/ID", "1").toString();
    m_agvIp = settings.value("AGV/IP", "127.0.0.1").toString();
    m_maxSpeed = settings.value("AGV/MaxSpeed", 100).toInt();
    // 文件夹路径
    m_resourceFolder = settings.value("Folder/resource", "/home/ruinap/config_qt/resource").toString();
    m_mapPngFolder = settings.value("Folder/mapPng", "/home/ruinap/map").toString();
    // 网络通讯
    m_commIp = settings.value("Network/CommIp", "host.docker.internal").toString();
    m_commPort = settings.value("Network/CommPort", 9001).toInt();
    m_rosbridgeIp = settings.value("Network/RosBridgeIp", "host.docker.internal").toString();
    m_rosbridgePort = settings.value("Network/RosBridgePort", 9090).toInt();
    m_serverIp = settings.value("Network/ServerIP", "192.168.1.1").toString();
    m_serverPort = settings.value("Network/ServerPort", 8080).toInt();
    // 系统选项
    m_autoConnect = settings.value("System/AutoConnect", false).toBool();
    m_debugMode = settings.value("System/DebugMode", false).toBool();
    m_fullScreen = settings.value("System/FullScreen", false).toBool();

    qDebug() << "ConfigManager: 配置已加载, AGV ID =" << m_agvId;
}

void ConfigManager::save()
{
    QSettings settings;

    // 车体参数
    settings.setValue("AGV/ID", m_agvId);
    settings.setValue("AGV/IP", m_agvIp);
    settings.setValue("AGV/MaxSpeed", m_maxSpeed);
    // 文件夹路径
    settings.setValue("Folder/resource", m_resourceFolder);
    settings.setValue("Folder/mapPng", m_mapPngFolder);
    // 网络通讯
    settings.setValue("Network/CommIp", m_commIp);
    settings.setValue("Network/CommPort", m_commPort);
    settings.setValue("Network/RosBridgeIp", m_rosbridgeIp);
    settings.setValue("Network/RosBridgePort", m_rosbridgePort);
    settings.setValue("Network/ServerIP", m_serverIp);
    settings.setValue("Network/ServerPort", m_serverPort);
    // 系统选项
    settings.setValue("System/AutoConnect", m_autoConnect);
    settings.setValue("System/DebugMode", m_debugMode);
    settings.setValue("System/FullScreen", m_fullScreen);

    settings.sync(); // 强制写入磁盘

    // 发送信号，通知所有界面更新
    emit configChanged();
    qDebug() << "ConfigManager: 配置已保存并通知所有组件";
}

// --- Getters 实现 ---
// 车体参数
QString ConfigManager::agvId() const { return m_agvId; }
QString ConfigManager::agvIp() const { return m_agvIp; }
int ConfigManager::maxSpeed() const { return m_maxSpeed; }
// 文件夹路径
QString ConfigManager::resourceFolder() const { return m_resourceFolder; }
QString ConfigManager::mapPngFolder() const { return m_mapPngFolder; }
// 网络通讯
QString ConfigManager::commIp() const { return m_commIp; }
int ConfigManager::commPort() const { return m_commPort; }
QString ConfigManager::rosBridgeIp() const { return m_rosbridgeIp; }
int ConfigManager::rosBridgePort() const { return m_rosbridgePort; }
QString ConfigManager::serverIp() const { return m_serverIp; }
int ConfigManager::serverPort() const { return m_serverPort; }
// 系统选项
bool ConfigManager::autoConnect() const { return m_autoConnect; }
bool ConfigManager::debugMode() const { return m_debugMode; }
bool ConfigManager::fullScreen() const { return m_fullScreen; }

// --- Setters 实现 ---
// 车体参数
void ConfigManager::setAgvId(const QString &id) { m_agvId = id; }
void ConfigManager::setAgvIp(const QString &ip) { m_agvIp = ip; }
void ConfigManager::setMaxSpeed(int speed) { m_maxSpeed = speed; }
// 文件夹路径
void ConfigManager::setResourceFolder(const QString &folder) { m_resourceFolder = folder; }
void ConfigManager::setMapPngFolder(const QString &folder) { m_mapPngFolder = folder; }
// 网络通讯
void ConfigManager::setCommIp(const QString &ip) { m_commIp = ip; }
void ConfigManager::setCommPort(int port) { m_commPort = port; }
void ConfigManager::setRosBridgeIp(const QString &ip) { m_rosbridgeIp = ip; }
void ConfigManager::setRosBridgePort(int port) { m_rosbridgePort = port; }
void ConfigManager::setServerIp(const QString &ip) { m_serverIp = ip; }
void ConfigManager::setServerPort(int port) { m_serverPort = port; }
// 系统选项
void ConfigManager::setAutoConnect(bool enable) { m_autoConnect = enable; }
void ConfigManager::setDebugMode(bool enable) { m_debugMode = enable; }
void ConfigManager::setFullScreen(bool enable) { m_fullScreen = enable; }