#ifndef MONITORINTERACTIONHANDLER_H
#define MONITORINTERACTIONHANDLER_H

#include <QObject>
#include <QPointF>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QWheelEvent>
#include "LogManager.h"

class MonitorWidget; // 前向声明

class MonitorInteractionHandler : public QObject
{
    Q_OBJECT
public:
    explicit MonitorInteractionHandler(MonitorWidget *parent);

    // 核心事件处理接口
    void handleMousePress(QMouseEvent *event);
    void handleMouseMove(QMouseEvent *event);
    void handleMouseRelease(QMouseEvent *event);
    void handleWheel(QWheelEvent *event);
    void handleTouch(QTouchEvent *event);

    // 状态重置
    void resetState();

signals:
    void hitFixedRelocation(bool state, int x, int y, int angle);

private:
    MonitorWidget *w; // 指向父组件，用于访问状态和触发 update

    // 日志管理器
    LogManager *logger = &LogManager::instance();

    // 内部记录变量
    QPointF m_lastMousePos;
    bool m_touchActive = false;
    bool m_isDraggingBig = false;
    bool m_isDraggingSmall = false;
    QPointF m_dragOffset;

    // 辅助函数：判断点是否在绘图区
    bool isInDrawingArea(const QPointF &pos);
};

#endif