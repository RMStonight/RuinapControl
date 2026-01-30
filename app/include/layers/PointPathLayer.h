#ifndef POINTPATHLAYER_H
#define POINTPATHLAYER_H

#include "BaseLayer.h"
#include <QVector>
#include <QPointF>
#include <QColor>
#include <QPainterPath>

// 定义路径信息结构体
struct MapPathData
{
    QPointF start;
    QPointF end;
    QPointF ctl1;
    QPointF ctl2;
    int type; // 1,4:直线; 2,5:二阶; 3,6:三阶
};

struct MapPointData
{
    QPointF pos;
    QString id;
    QColor color;
};

class PointPathLayer : public BaseLayer
{
public:
    double radius = 0.4;
    
    void updateData(const QVector<MapPointData> &points, const QVector<MapPathData> &paths)
    {
        m_points = points;
        m_paths = paths;
    }

    void draw(QPainter *painter) override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setRenderHint(QPainter::TextAntialiasing, true);

        // --- 1. 先绘制路径 (Path) ---
        drawPaths(painter);

        // --- 2. 再绘制点位 (Point) ---
        drawPoints(painter);

        painter->restore();
    }

private:
    void drawArrowOnPath(QPainter *painter, const QPainterPath &path)
    {
        // 获取路径中点的位置（0.5 表示 50% 处）
        qreal percent = 0.5;
        QPointF midPoint = path.pointAtPercent(percent); // 箭头的尖尖位置

        // 获取中点处的角度（切线方向）
        qreal angle = path.angleAtPercent(percent);

        painter->save();
        painter->translate(midPoint);
        painter->rotate(-angle); // QPainterPath 的角度是逆时针，需转换

        // 设置箭头样式
        QPen arrowPen(QColor(0, 255, 0));
        arrowPen.setCosmetic(true);
        arrowPen.setWidth(2);
        painter->setPen(arrowPen);
        painter->setBrush(QColor(0, 255, 0)); // 实心箭头

        // 绘制箭头（以 midPoint 为尖尖，向后延伸）
        // 箭头大小根据物理长度设定，例如 0.3 米
        double arrowLen = 0.3;
        double arrowWidth = 0.2;
        QPolygonF arrowHead;
        arrowHead << QPointF(0, 0)                         // 尖尖点（中点）
                  << QPointF(arrowLen, arrowWidth / 2.0)   // 后翼 1
                  << QPointF(arrowLen, -arrowWidth / 2.0); // 后翼 2

        painter->drawPolygon(arrowHead);
        painter->restore();
    }

    void drawPaths(QPainter *painter)
    {
        QPen pen(QColor(0, 255, 0, 150)); // 深灰色半透明
        pen.setWidth(1);
        pen.setCosmetic(true);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);

        for (const auto &path : m_paths)
        {
            QPainterPath qPath;
            // 注意：Y轴取负以适配地图坐标系
            QPointF pStart(path.start.x(), -path.start.y());
            QPointF pEnd(path.end.x(), -path.end.y());

            qPath.moveTo(pStart);

            if (path.type == 1 || path.type == 4)
            { // 直线
                qPath.lineTo(pEnd);
            }
            else if (path.type == 2 || path.type == 5)
            { // 二阶贝塞尔
                QPointF pCtl1(path.ctl1.x(), -path.ctl1.y());
                qPath.quadTo(pCtl1, pEnd);
            }
            else if (path.type == 3 || path.type == 6)
            { // 三阶贝塞尔
                QPointF pCtl1(path.ctl1.x(), -path.ctl1.y());
                QPointF pCtl2(path.ctl2.x(), -path.ctl2.y());
                qPath.cubicTo(pCtl1, pCtl2, pEnd);
            }
            painter->drawPath(qPath);

            drawArrowOnPath(painter, qPath);
        }
    }

    void drawPoints(QPainter *painter)
    {
        for (const auto &point : m_points)
        {
            QPointF center(point.pos.x(), -point.pos.y());

            painter->setBrush(point.color);
            QPen borderPen(Qt::white);
            borderPen.setWidth(1);
            borderPen.setCosmetic(true);
            painter->setPen(borderPen);
            painter->drawEllipse(center, radius, radius);

            // 绘制文字 ID (保持之前修正后的逻辑)
            painter->save();
            painter->translate(center);
            double internalScale = 100.0;
            painter->scale(1.0 / internalScale, 1.0 / internalScale);
            QFont font = painter->font();
            font.setPointSizeF(radius * internalScale * 0.6);
            font.setBold(true);
            painter->setFont(font);
            painter->setPen(Qt::white);
            QRectF textRect(-radius * internalScale, -radius * internalScale,
                            radius * internalScale * 2, radius * internalScale * 2);
            painter->drawText(textRect, Qt::AlignCenter, point.id);
            painter->restore();
        }
    }

private:
    QVector<MapPointData> m_points;
    QVector<MapPathData> m_paths;
};

#endif