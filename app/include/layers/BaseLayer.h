#ifndef BASELAYER_H
#define BASELAYER_H

#include <QPainter>

class BaseLayer {
public:
    virtual ~BaseLayer() = default;
    // 每个图层具体的绘制逻辑
    virtual void draw(QPainter *painter) = 0;
    
    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }

protected:
    bool m_visible = true;
};

#endif