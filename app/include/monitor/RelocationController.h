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
    explicit RelocationController(MonitorWidget *parent);

    // 业务接口
    void start();    // 进入重定位模式
    void finish();   // 确认并应用
    void cancel();   // 取消并退出

private:
    void exitMode(); // 统一退出逻辑

private:
    MonitorWidget *w;

    // 日志管理器
    LogManager *logger = &LogManager::instance();
};

#endif