#ifndef PERMISSIONMANAGER_H
#define PERMISSIONMANAGER_H

#include <QObject>
#include <QTimer>
#include <QDebug>
#include "AgvData.h"
#include "LogManager.h"

class PermissionManager : public QObject
{
    Q_OBJECT
public:
    static PermissionManager *instance()
    {
        static PermissionManager inst;
        return &inst;
    }

    // 当检测到任何用户活动时调用
    void handleActivity()
    {
        // 只有在非普通用户（管理员/开发者）时，计时才有意义
        // 假设 AgvData 中定义了 UserRole 枚举
        if (ConfigManager::instance()->currentUserRole() != UserRole::Operator)
        {
            // qDebug() << "HandleActivity, current is " << static_cast<int>(ConfigManager::instance()->currentUserRole()) << "role.";
            startMonitor();
        }
    }

    void startMonitor()
    {
        m_timeoutTimer.start();
    }

private slots:
    void onTimeout()
    {
        logger->log(QStringLiteral("PermissionManager"), spdlog::level::warn, QStringLiteral("User inactive for 5 s, reverting to Operator role."));
        // 执行自动降权逻辑
        ConfigManager::instance()->setCurrentUserRole(UserRole::Operator);

        // 发送信号或直接通过 AgvData 广播权限变化信号
        // emit permissionExpired();
    }

private:
    explicit PermissionManager(QObject *parent = nullptr) : QObject(parent)
    {
        m_timeoutTimer.setSingleShot(true);
        m_timeoutTimer.setInterval(5 * 1000); // 5 秒钟
        // m_timeoutTimer.setInterval(30 * 60 * 1000); // 30 分钟
        connect(&m_timeoutTimer, &QTimer::timeout, this, &PermissionManager::onTimeout);
    }

    // 日志管理器
    LogManager *logger = &LogManager::instance();

    QTimer m_timeoutTimer;
};

#endif