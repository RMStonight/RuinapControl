#ifndef MAPLAYER_H
#define MAPLAYER_H

#include "BaseLayer.h"

class MapLayer : public BaseLayer
{
public:
    void updateMap(const QPixmap &pixmap, double res, double ox, double oy)
    {
        m_pixmap = pixmap;
        m_res = res;
        m_ox = ox;
        m_oy = oy;
    }

    void draw(QPainter *painter) override
    {
        if (m_pixmap.isNull())
            return;

        painter->save();
        painter->translate(m_ox, m_oy);

        double w = m_pixmap.width() * m_res;
        double h = m_pixmap.height() * m_res;

        // 显式使用 QRectF 确保匹配重载列表
        QRectF targetRect(0.0, -h, w, h);
        QRectF sourceRect(m_pixmap.rect()); // 将 QRect 转换为 QRectF

        painter->drawPixmap(targetRect, m_pixmap, sourceRect);

        painter->restore();
    }

private:
    QPixmap m_pixmap;
    double m_res, m_ox, m_oy;
};

#endif