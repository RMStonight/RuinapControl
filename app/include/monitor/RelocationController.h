#ifndef RELOCATIONCONTROLLER_H
#define RELOCATIONCONTROLLER_H

#include <QObject>
#include <QPointF>
#include "LogManager.h"

class MonitorWidget;

class RelocationController : public QObject
{
    Q_OBJECT
public:
    // 定义重定位模式
    enum ReloMode
    {
        FreeMode, // 自由重定位
        FixedMode // 固定重定位
    };

    explicit RelocationController(MonitorWidget *parent);

    // 业务接口
    void start();  // 进入重定位模式
    void finish(); // 确认并应用
    void cancel(); // 取消并退出

    // 切换模式接口
    void switchMode();
    void setMode(ReloMode mode);
    ReloMode currentMode() const { return m_currentMode; }

private:
    void exitMode(); // 统一退出逻辑

private:
    MonitorWidget *w;
    ReloMode m_currentMode = FreeMode; // 默认为自由模式

    // 日志管理器
    LogManager *logger = &LogManager::instance();
};

#endif