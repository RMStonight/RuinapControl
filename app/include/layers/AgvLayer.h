#ifndef AGVLAYER_H
#define AGVLAYER_H

#include "BaseLayer.h"
#include <QtMath>
#include "utils/ConfigManager.h"
#include <QDebug>

class RobotLayer : public BaseLayer
{
public:
    RobotLayer() : m_agvScale(1.0) {}

    void setAgvScale(double scale) { m_agvScale = scale; }

    void updatePose(int x, int y, int angle)
    {
        m_x = x / 1000.0;
        m_y = y / 1000.0;
        m_rad = angle / 1000.0;
    }

    void draw(QPainter *painter) override
    {
        painter->save();

        // 坐标变换 (所有车型共用的世界坐标 -> 局部坐标变换)
        painter->translate(m_x, -m_y);
        painter->rotate(-qRadiansToDegrees(m_rad));
        painter->scale(1, -1);

        // 根据车型绘制具体的 AGV 形状
        int vehicleType = cfg->vehicleType();
        switch (vehicleType)
        {
        case 0:
            drawTriangleAgv(painter, m_agvScale);
            break;
        case 1:
            drawForkliftAgv(painter, m_agvScale);
            break;
        case 2:
            drawFourForkAgv(painter, m_agvScale);
            break;
        default:
            drawTriangleAgv(painter, m_agvScale); // 默认绘制三角形
            break;
        }

        painter->restore();
    }

private:
    /**
     * @brief 车型 0: 三角形 AGV 模型
     */
    void drawTriangleAgv(QPainter *painter, double m_agvScale)
    {
        // 绘制车体多边形
        QPolygonF shape;
        shape << QPointF(0.6 * m_agvScale, 0)
              << QPointF(-0.4 * m_agvScale, 0.4 * m_agvScale)
              << QPointF(-0.4 * m_agvScale, -0.4 * m_agvScale);

        painter->setBrush(QColor(0, 191, 255, 200)); // 半透明天蓝色
        painter->setPen(QPen(Qt::white, 0.02));      // 白色细边框
        painter->drawPolygon(shape);

        // 绘制车中心红点
        painter->setBrush(Qt::red);
        painter->setPen(Qt::NoPen);
        double dotRadius = 0.10 * m_agvScale;
        painter->drawEllipse(QPointF(0, 0), dotRadius, dotRadius);
    }
    /**
     * @brief 车型 1: 双叉叉车模型 (垂直长方形 + 靠拢的左侧货叉)
     * 旋转中心位于货叉中心
     */
    void drawForkliftAgv(QPainter *painter, double m_agvScale)
    {
        double L = 0.8 * m_agvScale; // 基准尺寸 (长方形的长边)
        double W = 0.6 * L;          // 长方形的短边
        double forkL = L;            // 货叉长度
        double forkW = 0.1 * L;      // 货叉宽度

        // --- 新增控制参数 ---
        // forkGapScale 控制货叉离中心线的距离
        // 0.0 表示两根货叉完全并拢在中心
        // 1.0 表示货叉位于长方形的最边缘
        double forkGapScale = 0.4;

        painter->setPen(QPen(Qt::white, 0.02));

        // --- 1. 绘制长方形车体 ---
        painter->setBrush(QColor(0, 191, 255, 200));
        QRectF bodyRect(0.5 * L, -L / 2.0, W, L);
        painter->drawRect(bodyRect);

        // --- 2. 绘制货叉 ---
        painter->setBrush(QColor(150, 150, 150, 255));

        // 重新计算 Y 轴偏移：基于车体半宽 (L/2) 乘以间距比例
        double forkYOffset = (L / 2.0 - forkW) * forkGapScale;

        // 上货叉 (Y 轴正向)
        painter->drawRect(QRectF(-0.5 * L, forkYOffset, forkL, forkW));
        // 下货叉 (Y 轴负向)
        painter->drawRect(QRectF(-0.5 * L, -forkYOffset - forkW, forkL, forkW));

        // --- 3. 绘制中心红点 ---
        painter->setBrush(Qt::red);
        painter->setPen(Qt::NoPen);
        double dotRadius = 0.10 * m_agvScale;
        painter->drawEllipse(QPointF(0, 0), dotRadius, dotRadius);
    }

    /**
     * @brief 车型 2: 四货叉叉车模型
     * 在双叉基础上向两侧等间距拓展，并自动加长车体以承载货叉
     */
    void drawFourForkAgv(QPainter *painter, double m_agvScale)
    {
        double L = 0.8 * m_agvScale; // 基准尺寸
        double forkL = L;            // 货叉长度
        double forkW = 0.1 * L;      // 货叉宽度
        double forkGapScale = 0.4;   // 原始间距比例

        // 1. 计算货叉位置
        // 基础偏移量（内侧两根货叉的 Y 坐标偏移）
        double innerYOffset = (L / 2.0 - forkW) * forkGapScale;
        // 计算两根货叉之间的中心间距，用于向外拓展
        // 间距 = 上货叉 Y - 下货叉 Y (顶部对顶部，所以要算上货叉宽度)
        double gap = (innerYOffset) - (-innerYOffset - forkW);

        // 四根货叉的 Y 轴起始点 (从上往下)
        double yForks[4];
        yForks[0] = innerYOffset + gap;    // 外侧上
        yForks[1] = innerYOffset;          // 内侧上
        yForks[2] = -innerYOffset - forkW; // 内侧下
        yForks[3] = yForks[2] - gap;       // 外侧下

        // 2. 根据最外侧货叉位置动态调整车体长度 (L_ext)
        // 确保车体覆盖所有货叉的宽度，增加少量边距 (forkW)
        double L_ext = (yForks[0] + forkW) * 2.0;
        double W = 0.6 * L; // 车体厚度保持比例

        painter->setPen(QPen(Qt::white, 0.02));

        // --- 1. 绘制长方形车体 ---
        // 使用扩展后的 L_ext 作为 Y 轴长度
        painter->setBrush(QColor(0, 191, 255, 200));
        QRectF bodyRect(0.5 * L, -L_ext / 2.0, W, L_ext);
        painter->drawRect(bodyRect);

        // --- 2. 绘制四根货叉 ---
        painter->setBrush(QColor(150, 150, 150, 255));
        for (int i = 0; i < 4; ++i)
        {
            painter->drawRect(QRectF(-0.5 * L, yForks[i], forkL, forkW));
        }

        // --- 3. 绘制中心红点 ---
        painter->setBrush(Qt::red);
        painter->setPen(Qt::NoPen);
        double dotRadius = 0.10 * m_agvScale;
        painter->drawEllipse(QPointF(0, 0), dotRadius, dotRadius);
    }

private:
    double m_x = 0, m_y = 0, m_rad = 0;
    double m_agvScale;
    ConfigManager *cfg = ConfigManager::instance();
};

#endif