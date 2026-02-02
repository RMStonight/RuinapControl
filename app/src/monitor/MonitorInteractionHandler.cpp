#include "monitor/MonitorInteractionHandler.h"
#include "components/MonitorWidget.h"
#include "layers/RelocationLayer.h" 
#include "layers/AgvLayer.h"        
#include <QLineF>
#include <QtMath>

MonitorInteractionHandler::MonitorInteractionHandler(MonitorWidget *parent)
    : QObject(parent), w(parent) {}

bool MonitorInteractionHandler::isInDrawingArea(const QPointF &pos)
{
    // 这里的 w->getDrawingWidth() 现在可以访问了
    return pos.x() < w->getDrawingWidth();
}

void MonitorInteractionHandler::handleMousePress(QMouseEvent *event)
{
    // 访问私有变量 m_touchActive
    if (m_touchActive || !isInDrawingArea(event->localPos()))
        return;

    // 访问私有变量 m_isRelocating 和 m_reloLayer
    if (w->m_isRelocating && w->m_reloLayer && w->m_reloLayer->isVisible())
    {
        QPointF worldPos = (event->localPos() - w->m_offset) / w->m_scale;
        if (w->m_reloLayer->isHitSmallCircle(worldPos))
        {
            m_isDraggingSmall = true;
            return;
        }
        else if (w->m_reloLayer->isHitBigCircle(worldPos))
        {
            m_isDraggingBig = true;
            m_dragOffset = w->m_reloLayer->pos() - worldPos;
            return;
        }
    }

    if (event->button() == Qt::LeftButton)
    {
        m_lastMousePos = event->localPos();
        // 调用私有方法 checkPointClick
        w->checkPointClick(event->localPos());
    }
    m_lastMousePos = event->localPos();
}

void MonitorInteractionHandler::handleMouseMove(QMouseEvent *event)
{
    if (m_touchActive || !isInDrawingArea(event->localPos()))
        return;

    QPointF worldPos = (event->localPos() - w->m_offset) / w->m_scale;

    if (m_isDraggingSmall)
    {
        w->m_reloLayer->updateAngle(worldPos);
        // 同步更新 AGV 位姿
        w->m_agvLayer->updatePose(w->m_reloLayer->pos().x() * 1000,
                                  -w->m_reloLayer->pos().y() * 1000,
                                  w->m_reloLayer->getAngle() * 1000);
        w->update();
    }
    else if (m_isDraggingBig)
    {
        w->m_reloLayer->setPos(worldPos + m_dragOffset);
        w->m_agvLayer->updatePose(w->m_reloLayer->pos().x() * 1000,
                                  -w->m_reloLayer->pos().y() * 1000,
                                  w->m_reloLayer->getAngle() * 1000);
        w->update();
    }
    else if (event->buttons() & Qt::LeftButton)
    {
        // 地图平移：修改私有变量 m_offset
        w->m_offset += (event->localPos() - m_lastMousePos);
        m_lastMousePos = event->localPos();
        w->update();
    }
}

void MonitorInteractionHandler::handleMouseRelease(QMouseEvent *event)
{
    Q_UNUSED(event);
    m_isDraggingSmall = false;
    m_isDraggingBig = false;
}

void MonitorInteractionHandler::handleWheel(QWheelEvent *event)
{
    if (!isInDrawingArea(event->position()))
        return;

    QPointF mousePos = QPointF(event->position());
    QPointF worldPosBeforeZoom = (mousePos - w->m_offset) / w->m_scale;

    double factor = (event->angleDelta().y() < 0) ? (1.0 / 1.1) : 1.1;
    // 使用 qBound 限制 m_scale 范围
    double newScale = qBound(1.0, w->m_scale * factor, 500.0);

    w->m_scale = newScale;
    w->m_offset = mousePos - (worldPosBeforeZoom * w->m_scale);
    w->update();
}

void MonitorInteractionHandler::handleTouch(QTouchEvent *event)
{
    const QList<QTouchEvent::TouchPoint> &points = event->touchPoints();
    if (points.isEmpty())
        return;

    if (event->type() == QEvent::TouchBegin)
    {
        if (!isInDrawingArea(points.first().pos()))
        {
            m_touchActive = false;
            return;
        }
        m_touchActive = true;

        if (points.count() == 1)
        {
            QPointF worldPos = (points.first().pos() - w->m_offset) / w->m_scale;
            if (w->m_isRelocating)
            {
                if (w->m_reloLayer->isHitSmallCircle(worldPos))
                {
                    m_isDraggingSmall = true;
                }
                else if (w->m_reloLayer->isHitBigCircle(worldPos))
                {
                    m_isDraggingBig = true;
                    m_dragOffset = w->m_reloLayer->pos() - worldPos;
                }
            }
            if (!m_isDraggingSmall && !m_isDraggingBig)
            {
                w->checkPointClick(points.first().pos());
            }
        }
    }

    if (!m_touchActive)
        return;

    if (points.count() == 1)
    { // 单指逻辑
        if (event->type() == QEvent::TouchUpdate)
        {
            QPointF worldPos = (points.first().pos() - w->m_offset) / w->m_scale;
            if (m_isDraggingSmall)
            {
                w->m_reloLayer->updateAngle(worldPos);
            }
            else if (m_isDraggingBig)
            {
                w->m_reloLayer->setPos(worldPos + m_dragOffset);
            }
            else
            {
                w->m_offset += (points.first().pos() - points.first().lastPos());
            }

            if (m_isDraggingSmall || m_isDraggingBig)
            {
                w->m_agvLayer->updatePose(w->m_reloLayer->pos().x() * 1000,
                                          -w->m_reloLayer->pos().y() * 1000,
                                          w->m_reloLayer->getAngle() * 1000);
            }
            w->update();
        }
    }
    else if (points.count() == 2)
    { // 双指缩放逻辑
        QPointF p1 = points[0].pos(), p2 = points[1].pos();
        QPointF p1L = points[0].lastPos(), p2L = points[1].lastPos();
        double curD = QLineF(p1, p2).length(), lastD = QLineF(p1L, p2L).length();

        if (lastD > 0.1)
        {
            QPointF curCenter = (p1 + p2) / 2.0;
            QPointF worldPos = (curCenter - w->m_offset) / w->m_scale;
            w->m_scale = qBound(1.0, w->m_scale * (curD / lastD), 500.0);
            w->m_offset = curCenter - (worldPos * w->m_scale) + (curCenter - (p1L + p2L) / 2.0);
            w->update();
        }
    }

    if (event->type() == QEvent::TouchEnd || event->type() == QEvent::TouchCancel)
    {
        resetState();
    }
}

void MonitorInteractionHandler::resetState()
{
    m_isDraggingSmall = false;
    m_isDraggingBig = false;
    m_touchActive = false;
}