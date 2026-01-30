#ifndef GRIDLAYER_H
#define GRIDLAYER_H

#include "BaseLayer.h"

class GridLayer : public BaseLayer {
public:
    void draw(QPainter *painter) override {
        painter->save();
        painter->setPen(QPen(QColor(200, 200, 200), 0)); // 浅灰色网格
        painter->drawLine(QPointF(-1, 0), QPointF(1, 0));
        painter->drawLine(QPointF(0, -1), QPointF(0, 1));
        painter->restore();
    }
};

#endif