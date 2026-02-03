#ifndef GLOBALEVENTFILTER_H
#define GLOBALEVENTFILTER_H

#include <QObject>
#include <QEvent>
#include <QMouseEvent>
#include <QTouchEvent>
#include "PermissionManager.h"

class GlobalEventFilter : public QObject
{
    Q_OBJECT
protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        // 捕获鼠标按下、移动或触摸事件
        if (event->type() == QEvent::MouseButtonPress || 
            event->type() == QEvent::MouseMove ||
            event->type() == QEvent::TouchBegin) 
        {
            PermissionManager::instance()->handleActivity();
        }
        return QObject::eventFilter(obj, event); // 继续传递事件，不拦截业务
    }
};

#endif