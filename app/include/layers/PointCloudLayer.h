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

    void updatePoints(const QVector<QPointF> &points) {
        m_points = points;
    }

    void draw(QPainter *painter) override {
        if (m_points.isEmpty()) return;

        painter->save();
        
        // 1. 处理 ROS 到 Qt 画布的镜像关系 (Y轴翻转)
        painter->scale(1, -1); 

        // 2. 设置画笔与画刷
        // 使用画刷填充圆形，不设置画笔边框可以避免缩放带来的线条畸变
        painter->setPen(Qt::NoPen);
        painter->setBrush(Qt::red);

        // 3. 绘制点云
        // 由于 painter 已经应用了全局 m_scale，
        // 我们需要根据缩放比例计算一个固定的“物理尺寸”圆点。
        // 假设我们希望点在屏幕上始终是 3 像素宽：
        // 物理半径 = 期望像素 / m_scale
        // 注意：这里需要获取当前 painter 的变换矩阵来动态计算，或者简单传参
        
        double pointRadius = 2.0 / painter->transform().m11(); // 动态计算保持像素大小不变

        for (const QPointF &p : m_points) {
            // 绘制圆形而非 drawPoint，解决“横线”视觉问题
            painter->drawEllipse(p, pointRadius, pointRadius);
        }

        painter->restore();
    }

private:
    QVector<QPointF> m_points;
};

#endif