#ifndef RELOCATIONLAYER_H
#define RELOCATIONLAYER_H

#include "BaseLayer.h"
#include <QPointF>
#include <QtMath>
#include <QDebug>

class RelocationLayer : public BaseLayer {
public:
    RelocationLayer() {
        m_visible = false; // 默认隐藏
    }

    // 设置在世界坐标系（米）中的位置
    void setPos(const QPointF &pos) { m_center = pos; }
    void setAngle(double angle) { m_angle = angle; }
    QPointF pos() const { return m_center; }

    // 获取小圆对应的角度 (弧度，ROS 惯例：x轴正方向为0，逆时针为正)
    double getAngle() const { return m_angle; }

    void draw(QPainter *painter) override {
        painter->save();
        
        // 1. 绘制大圆 (淡绿色半透明)
        painter->setBrush(QColor(144, 238, 144, 100)); 
        painter->setPen(QPen(Qt::green, 0.05));
        painter->drawEllipse(m_center, m_bigRadius, m_bigRadius);

        // 2. 计算并绘制小圆 (红色)
        // 小圆中心 = 大圆中心 + 向量(半径差 * cos/sin 角度)
        double rDiff = m_bigRadius - m_smallRadius;
        QPointF smallCenter(m_center.x() + rDiff * qCos(m_angle),
                            m_center.y() - rDiff * qSin(m_angle)); // Qt y轴向下，故减去sin

        painter->setBrush(Qt::red);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(smallCenter, m_smallRadius, m_smallRadius);

        painter->restore();
    }

    // --- 交互判断逻辑 ---

    // 检查是否点中了小圆 (用于旋转控制)
    bool isHitSmallCircle(const QPointF &worldPos) {
        double rDiff = m_bigRadius - m_smallRadius;
        QPointF smallCenter(m_center.x() + rDiff * qCos(m_angle),
                            m_center.y() - rDiff * qSin(m_angle));
        return QLineF(worldPos, smallCenter).length() < m_smallRadius * 1.5; // 适当增加判定范围
    }

    // 检查是否点中了大圆 (用于移动控制)
    bool isHitBigCircle(const QPointF &worldPos) {
        return QLineF(worldPos, m_center).length() < m_bigRadius;
    }

    // 更新角度（基于点击的世界坐标）
    void updateAngle(const QPointF &worldPos) {
        QPointF delta = worldPos - m_center;
        // atan2(y, x), 注意 Qt y 轴向下，需要取反匹配 ROS 惯例
        m_angle = qAtan2(-delta.y(), delta.x());
    }

private:
    QPointF m_center = QPointF(0, 0); // 世界坐标 (m)
    double m_angle = 0;               // 弧度
    const double m_bigRadius = 2.0;   // 大圆半径 1米
    const double m_smallRadius = 0.4; // 小圆半径 0.2米
};

#endif