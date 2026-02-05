#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QDir>
#include <QCoreApplication>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/callback_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

class LogManager : public QObject
{
    Q_OBJECT
public:
    static LogManager &instance()
    {
        static LogManager inst;
        return inst;
    }

    void init(QString logPath, bool debugMode)
    {
        try
        {
            // 路径初始化逻辑保持不变
            if (logPath.isEmpty())
            {
                logPath = QCoreApplication::applicationDirPath() + "/logs";
            }
            QDir dir;
            if (!dir.exists(logPath))
            {
                dir.mkpath(logPath);
            }
            QString fullPath = QDir(logPath).filePath("qt.log");

            // 准备一个 Sink 列表
            std::vector<spdlog::sink_ptr> sinks;

            // 1. 始终添加旋转文件 Sink
            sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                fullPath.toStdString(),
                m_maxFileSize,
                m_maxFiles));

            // 2. 始终添加回调 Sink (用于 UI 显示)
            auto ui_sink = std::make_shared<spdlog::sinks::callback_sink_mt>([this](const spdlog::details::log_msg &msg)
                                                                             {
            QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
            auto view = spdlog::level::to_string_view(msg.level);
            QString level = QString::fromUtf8(view.data(), static_cast<int>(view.size()));
            QString content = QString::fromUtf8(msg.payload.data(), static_cast<int>(msg.payload.size()));
            emit newLogEntry(time, level, content); });
            sinks.push_back(ui_sink);

            // 3. 【关键修改】根据 debugMode 决定是否添加控制台 Sink
            if (debugMode)
            {
                auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                sinks.push_back(console_sink);
            }

            // 使用准备好的 sinks 列表创建 Logger
            auto logger = std::make_shared<spdlog::logger>("", sinks.begin(), sinks.end());
            spdlog::set_default_logger(logger);

            // 设置全局格式
            // spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
            spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] %^[%l]%$ %v");

            // 其他优化项保持不变
            spdlog::flush_every(std::chrono::seconds(3));
            spdlog::set_level(spdlog::level::debug);
            spdlog::flush_on(spdlog::level::warn);
        }
        catch (const spdlog::spdlog_ex &ex)
        {
            fprintf(stderr, "Log initialization failed: %s\n", ex.what());
        }
    }

    // --- 优化 2：支持自定义抬头的通用记录方法 ---
    /**
     * @brief 记录带有自定义模块抬头的日志
     * @param module 模块名，如 "CAN", "ROS", "UI"
     * @param level 日志等级
     * @param msg 日志内容
     */
    void log(const QString &module, spdlog::level::level_enum level, const QString &msg)
    {
        // 格式化为：[模块名] 实际内容
        std::string finalMsg = fmt::format("[{}] {}", module.toStdString(), msg.toStdString());
        spdlog::default_logger_raw()->log(level, finalMsg);
    }

signals:
    void newLogEntry(const QString &time, const QString &level, const QString &msg);

private:
    explicit LogManager(QObject *parent = nullptr) : QObject(parent) {}

    // 单个日志文件的最大大小：10MB
    const size_t m_maxFileSize = 1024 * 1024 * 10;

    // 最多保留的日志文件数量：5个
    const size_t m_maxFiles = 5;
};

#endif