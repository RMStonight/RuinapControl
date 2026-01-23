#include "utils/NetworkCheckThread.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>

NetworkCheckThread::NetworkCheckThread(QObject *parent)
    : QThread(parent), m_stop(false), m_serverIp("192.168.1.1") // 默认服务器IP
{
}

NetworkCheckThread::~NetworkCheckThread()
{
    stop();
    wait();
}

void NetworkCheckThread::stop()
{
    QMutexLocker locker(&m_mutex);
    m_stop = true;
}

void NetworkCheckThread::setServerIp(const QString &ip)
{
    QMutexLocker locker(&m_mutex);
    m_serverIp = ip;
}

void NetworkCheckThread::loadConfig(const QString &jsonPath)
{
    QMutexLocker locker(&m_mutex);
    m_deviceList.clear();

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open eth_ip.json:" << jsonPath;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        QJsonObject root = doc.object();
        if (root.contains("list") && root["list"].isArray()) {
            QJsonArray list = root["list"].toArray();
            for (const QJsonValue &val : list) {
                QJsonObject obj = val.toObject();
                DeviceNode node;
                node.name = obj["name"].toString();
                node.ip = obj["ip"].toString();
                if (!node.ip.isEmpty()) {
                    m_deviceList.append(node);
                }
            }
        }
    }
}

double NetworkCheckThread::pingAddress(const QString &ip)
{
    // 根据操作系统选择 ping 参数
    // Windows: -n 1 (次数), -w 1000 (超时ms)
    // Linux:   -c 1 (次数), -W 1 (超时s)
#ifdef Q_OS_WIN
    QString program = "ping";
    QStringList arguments;
    arguments << "-n" << "1" << "-w" << "500" << ip; // 500ms超时，加快检测速度
#else
    QString program = "ping";
    QStringList arguments;
    arguments << "-c" << "1" << "-W" << "1" << "-n" << ip;
#endif

    QProcess process;
    process.start(program, arguments);
    process.waitForFinished();

    if (process.exitCode() != 0) {
        return -1.0;
    }

    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    
    // 正则提取时间 time=2ms 或 time<1ms
    // Windows: time=2ms
    // Linux: time=2.12 ms
    QRegularExpression re("time[=<]([\\d\\.]+)");
    QRegularExpressionMatch match = re.match(output);
    
    if (match.hasMatch()) {
        return match.captured(1).toDouble();
    }

    // 某些情况ping通了但没抓到时间（很少见），认为是0ms
    return 0.1; 
}

void NetworkCheckThread::run()
{
    while (true) {
        {
            QMutexLocker locker(&m_mutex);
            if (m_stop) break;
        }

        QString currentServerIp;
        QList<DeviceNode> currentList;

        {
            QMutexLocker locker(&m_mutex);
            currentServerIp = m_serverIp;
            currentList = m_deviceList;
        }

        // 1. 优先 Ping 服务器
        double serverDelay = pingAddress(currentServerIp);
        
        // 2. 如果服务器不通 (通常服务器最重要，但按需求我们先检查列表)
        // 需求：当eth_ip中的成员都能正常ping通时，显示与服务器的延迟。
        // 如果 eth_ip 成员不通，显示 xxx离线。
        
        QString errorName = "";
        bool listAllOk = true;

        // 检查 JSON 列表
        for (const auto &node : currentList) {
            {
                QMutexLocker locker(&m_mutex);
                if (m_stop) return; // 快速退出
            }
            
            double delay = pingAddress(node.ip);
            if (delay < 0) {
                errorName = node.name;
                listAllOk = false;
                break; // 找到第一个离线即可退出循环
            }
        }

        if (listAllOk) {
            // 列表都好了，检查服务器
            if (serverDelay >= 0) {
                // 全部正常
                emit statusUpdate(QString("%1ms").arg(int(serverDelay)), true);
            } else {
                // 列表好了，但服务器挂了
                emit statusUpdate("服务器离线", false);
            }
        } else {
            // 列表里有设备离线
            emit statusUpdate(QString("%1离线").arg(errorName), false);
        }

        // 休眠 3 秒再进行下一轮检测
        sleep(3);
    }
}