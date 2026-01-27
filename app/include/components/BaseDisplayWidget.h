#ifndef BASEDISPLAYWIDGET_H
#define BASEDISPLAYWIDGET_H

#include <QWidget>
#include "OptionalInfoWidget.h"

class BaseDisplayWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BaseDisplayWidget(QWidget *parent = nullptr) : QWidget(parent) {}

    // 统一设置侧边栏并管理生命周期
    virtual void setSharedOptionalInfo(OptionalInfoWidget *widget) {
        if (!widget) return;
        m_currentSideBar = widget;
        widget->setParent(this);
        widget->show();
        updateSideBarPosition();
    }

protected:
    // 获取当前有效的左侧绘图区边界 (x 坐标)
    int getDrawingWidth() const {
        if (m_currentSideBar && m_currentSideBar->isVisible()) {
            return m_currentSideBar->x();
        }
        // 默认 3/4 宽度，防止侧边栏未初始化时的视觉跳变
        return width() * 3 / 4; 
    }

    // 计算并设置侧边栏位置（占右侧 1/4）
    void updateSideBarPosition() {
        if (m_currentSideBar) {
            int sideBarWidth = this->width() / 4;
            m_currentSideBar->setGeometry(this->width() - sideBarWidth, 0, sideBarWidth, this->height());
        }
    }

    // 响应窗口大小改变
    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        updateSideBarPosition();
    }

    OptionalInfoWidget *m_currentSideBar = nullptr;
};

#endif // BASEDISPLAYWIDGET_H