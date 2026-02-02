#ifndef POINTCLOUDLAYER_H
#define POINTCLOUDLAYER_H

#include "BaseLayer.h"
#include <QVector>
#include <QPointF>
#include <QPainter>

class PointCloudLayer : public BaseLayer
{
public:
    PointCloudLayer() {}

    void updatePoints(const QVector<QPointF> &points)
    {
        m_points = points;
    }

    // 进入重定位时，将世界坐标点转换为相对于 AGV 的局部坐标点
    void lockToLocal(const QPointF &agvPos, double agvRad)
    {
        m_localPoints.clear();
        for (const QPointF &worldP : m_points)
        {
            // 1. 平移到原点
            double dx = worldP.x() - agvPos.x();
            double dy = worldP.y() - agvPos.y();
            // 2. 逆旋转 (x' = xcos + ysin, y' = -xsin + ycos)
            double lx = dx * qCos(agvRad) + dy * qSin(agvRad);
            double ly = -dx * qSin(agvRad) + dy * qCos(agvRad);
            m_localPoints.append(QPointF(lx, ly));
        }
        m_isLocked = true;
    }

    void unlock() { m_isLocked = false; }

    // 重写 draw，支持局部坐标绘制
    void draw(QPainter *painter) override
    {
        if (m_isLocked || m_points.isEmpty())
            return;

        painter->save();
        // 同样，避免直接 scale 轴，改为在绘制点时取反 y
        painter->setPen(Qt::NoPen);
        painter->setBrush(Qt::red);

        double currentScale = qSqrt(qAbs(painter->transform().determinant()));
        double pointRadius = 2.0 / currentScale;

        for (const QPointF &p : m_points)
        {
            // p 为世界坐标 (ROS 规范：y向上)
            painter->drawEllipse(QPointF(p.x(), -p.y()), pointRadius, pointRadius);
        }
        painter->restore();
    }

    // 提供给外部：直接绘制局部点（由外部 Painter 决定 AGV 位姿）
    void drawLocal(QPainter *painter)
    {
        // 外部 painter 已经移到了 AGV 中心并旋转了角度
        painter->save();

        // 1. 核心修正：绝对不要在这里调用 painter->scale(1, -1)！

        // 2. 禁用画笔，防止出现黑色线团
        painter->setPen(Qt::NoPen);
        painter->setBrush(Qt::red);

        // 3. 稳健地计算半径：使用 transform 的行列式或平均缩放比例
        // 这样即便旋转，半径也保持恒定
        double currentScale = qSqrt(qAbs(painter->transform().determinant()));
        double pointRadius = 2.0 / currentScale;

        for (const QPointF &p : m_localPoints)
        {
            // 4. 手动翻转坐标点的 Y 值，以适配 Qt 坐标系 (y向下)
            // 而不是去翻转整个 Painter 坐标轴
            painter->drawEllipse(QPointF(p.x(), -p.y()), pointRadius, pointRadius);
        }
        painter->restore();
    }

private:
    QVector<QPointF> m_points;      // 世界坐标点
    QVector<QPointF> m_localPoints; // 局部坐标点
    bool m_isLocked = false;
};

#endif