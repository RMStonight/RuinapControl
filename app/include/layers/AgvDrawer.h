#ifndef AGVDRAWER_H
#define AGVDRAWER_H

#include <QPainter>
#include <QPolygonF>
#include <QRectF>

class AgvDrawer
{
public:
    /**
     * @brief 统一绘制入口
     * @param painter 绘图句柄
     * @param vehicleType 车型
     * @param color 车体颜色（允许不同图层传入不同透明度或色调）
     * @param scale 缩放比例
     */
    static void draw(QPainter *painter, int vehicleType, const QColor &color, double scale = 1.0)
    {
        switch (vehicleType)
        {
        case 0:
            drawTriangleAgv(painter, color, scale);
            break;
        case 1:
            drawForkliftAgv(painter, color, scale);
            break;
        case 2:
            drawFourForkAgv(painter, color, scale);
            break;
        default:
            drawTriangleAgv(painter, color, scale);
            break;
        }

        // 绘制通用的中心红点
        painter->setBrush(Qt::red);
        painter->setPen(Qt::NoPen);
        double dotRadius = 0.10 * scale;
        painter->drawEllipse(QPointF(0, 0), dotRadius, dotRadius);
    }

private:
    /**
     * @brief 车型 0: 三角形 AGV 模型
     */
    static void drawTriangleAgv(QPainter *painter, const QColor &color, double scale)
    {
        QPolygonF shape;
        shape << QPointF(0.6 * scale, 0)
              << QPointF(-0.4 * scale, 0.4 * scale)
              << QPointF(-0.4 * scale, -0.4 * scale);
        painter->setBrush(color);
        painter->setPen(QPen(Qt::white, 0.02));
        painter->drawPolygon(shape);
    }

    /**
     * @brief 车型 1: 双叉叉车模型 (垂直长方形 + 靠拢的左侧货叉)
     * 旋转中心位于货叉中心
     */
    static void drawForkliftAgv(QPainter *painter, const QColor &color, double scale)
    {
        double L = 0.8 * scale;
        double W = 0.6 * L;
        double forkL = L;
        double forkW = 0.1 * L;
        double forkGapScale = 0.4;

        painter->setPen(QPen(Qt::white, 0.02));
        painter->setBrush(color);
        painter->drawRect(QRectF(0.5 * L, -L / 2.0, W, L));

        painter->setBrush(QColor(150, 150, 150, color.alpha())); // 货叉随车体透明度
        double forkYOffset = (L / 2.0 - forkW) * forkGapScale;
        painter->drawRect(QRectF(-0.5 * L, forkYOffset, forkL, forkW));
        painter->drawRect(QRectF(-0.5 * L, -forkYOffset - forkW, forkL, forkW));
    }

    /**
     * @brief 车型 2: 四货叉叉车模型
     * 在双叉基础上向两侧等间距拓展，并自动加长车体以承载货叉
     */
    static void drawFourForkAgv(QPainter *painter, const QColor &color, double scale)
    {
        double L = 0.8 * scale;
        double forkL = L;
        double forkW = 0.1 * L;
        double forkGapScale = 0.4;

        double innerYOffset = (L / 2.0 - forkW) * forkGapScale;
        double gap = (innerYOffset) - (-innerYOffset - forkW);
        double yForks[4] = {innerYOffset + gap, innerYOffset, -innerYOffset - forkW, -innerYOffset - forkW - gap};

        double L_ext = (yForks[0] + forkW) * 2.0;
        double W = 0.6 * L;

        painter->setPen(QPen(Qt::white, 0.02));
        painter->setBrush(color);
        painter->drawRect(QRectF(0.5 * L, -L_ext / 2.0, W, L_ext));

        painter->setBrush(QColor(150, 150, 150, color.alpha()));
        for (int i = 0; i < 4; ++i)
        {
            painter->drawRect(QRectF(-0.5 * L, yForks[i], forkL, forkW));
        }
    }
};

#endif