#include "utils/NetworkCheckThread.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QRegularExpression>

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
    if (!file.open(QIODevice::ReadOnly))
    {
        logger->log(QStringLiteral("NetworkCheckThread"), spdlog::level::err, QStringLiteral("Failed to open eth_ip.json: %1").arg(jsonPath));
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject())
    {
        QJsonObject root = doc.object();
        if (root.contains("list") && root["list"].isArray())
        {
            QJsonArray list = root["list"].toArray();
            for (const QJsonValue &val : list)
            {
                QJsonObject obj = val.toObject();
                DeviceNode node;
                node.name = obj["name"].toString();
                node.ip = obj["ip"].toString();
                if (!node.ip.isEmpty())
                {
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

    if (process.exitCode() != 0)
    {
        return -1.0;
    }

    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());

    // 正则提取时间 time=2ms 或 time<1ms
    // Windows: time=2ms
    // Linux: time=2.12 ms
    QRegularExpression re("time[=<]([\\d\\.]+)");
    QRegularExpressionMatch match = re.match(output);

    if (match.hasMatch())
    {
        return match.captured(1).toDouble();
    }

    // 某些情况ping通了但没抓到时间（很少见），认为是0ms
    return 0.1;
}

void NetworkCheckThread::run()
{
    while (true)
    {
        {
            QMutexLocker locker(&m_mutex);
            if (m_stop)
                break;
        }

        QString currentServerIp;
        QList<DeviceNode> currentList;

        {
            QMutexLocker locker(&m_mutex);
            currentServerIp = m_serverIp;
            currentList = m_deviceList;
        }

        // 清空上一轮状态（可选，如果列表动态变化建议清空）
        m_networkStatusMap.clear();

        // 1. 检测服务器
        double serverDelay = pingAddress(currentServerIp);
        QString serverStatus = (serverDelay >= 0) ? QString("%1ms").arg(int(serverDelay)) : QStringLiteral("离线");
        m_networkStatusMap.insert(QStringLiteral("服务器"), serverStatus);

        // 2. 检测 eth_ip.json 中的成员
        // 需求：当eth_ip中的成员都能正常ping通时，显示与服务器的延迟。
        // 如果 eth_ip 成员不通，显示 xxx离线。
        QString firstErrorName = "";
        bool listAllOk = true;

        // 检查 JSON 列表
        for (const auto &node : currentList)
        {
            {
                QMutexLocker locker(&m_mutex);
                if (m_stop)
                    return;
            }

            double delay = pingAddress(node.ip);
            QString status;
            if (delay >= 0)
            {
                status = QString("%1ms").arg(int(delay));
            }
            else
            {
                status = QStringLiteral("离线");
                if (listAllOk)
                {
                    firstErrorName = node.name;
                    listAllOk = false;
                }
            }
            m_networkStatusMap.insert(node.name, status);
        }

        // 3. 更新 UI 信号 (维持原有业务逻辑：有设备离线优先显示设备名)
        if (listAllOk)
        {
            if (serverDelay >= 0)
            {
                emit statusUpdate(m_networkStatusMap[QStringLiteral("服务器")], true);
            }
            else
            {
                emit statusUpdate(QStringLiteral("服务器离线"), false);
            }
        }
        else
        {
            emit statusUpdate(QString("%1离线").arg(firstErrorName), false);
        }

        // 4. 执行一次完整日志记录
        logNetworkSummary();

        // 休眠 3 秒再进行下一轮检测
        sleep(3);
    }
}

void NetworkCheckThread::logNetworkSummary()
{
    QStringList details;
    QMapIterator<QString, QString> i(m_networkStatusMap);
    while (i.hasNext())
    {
        i.next();
        details << QString("%1:%2").arg(i.key(), i.value());
    }

    // 组合成一条日志：[NET] 服务器:4ms; 单片机:1ms; 导航雷达:离线
    QString logMsg = details.join("; ");

    // 使用您之前定义的 LogManager 记录
    logger->log(QStringLiteral("NetworkCheckThread"), spdlog::level::info, logMsg);
}