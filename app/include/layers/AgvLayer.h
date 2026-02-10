#ifndef AGVLAYER_H
#define AGVLAYER_H

#include "BaseLayer.h"
#include <QtMath>
#include "utils/ConfigManager.h"
#include "AgvDrawer.h"

class AgvLayer : public BaseLayer
{
public:
    AgvLayer() : m_agvScale(1.0) {}

    void setAgvScale(double scale) { m_agvScale = scale; }

    void updatePose(int x, int y, int angle)
    {
        m_x = x / 1000.0;
        m_y = y / 1000.0;
        m_rad = angle / 1000.0;
    }

    QPointF getPos()
    {
        return QPointF(m_x, m_y);
    }

    double getAngle()
    {
        return m_rad;
    }

    void draw(QPainter *painter) override
    {
        painter->save();

        // 坐标变换 (所有车型共用的世界坐标 -> 局部坐标变换)
        painter->translate(m_x, -m_y);
        painter->rotate(-qRadiansToDegrees(m_rad));
        painter->scale(1, -1);

        // 根据车型绘制具体的 AGV 形状
        AgvDrawer::draw(painter, cfg->vehicleType(), QColor(0, 191, 255, 200), m_agvScale);

        painter->restore();
    }

private:
    double m_x = 0, m_y = 0, m_rad = 0;
    double m_agvScale;
    ConfigManager *cfg = ConfigManager::instance();
};

#endif