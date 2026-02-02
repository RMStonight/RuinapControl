#include "monitor/RelocationController.h"
#include "components/MonitorWidget.h"
#include "layers/AgvLayer.h"
#include "layers/RelocationLayer.h"
#include "layers/PointCloudLayer.h"
#include <QPushButton>

RelocationController::RelocationController(MonitorWidget *parent)
    : QObject(parent), w(parent) {}

void RelocationController::start()
{
    w->m_isRelocating = true;
    w->m_reloLayer->setVisible(true);

    // 控制按钮显隐
    w->m_reloBtn->hide();
    w->m_confirmBtn->show();
    w->m_cancelBtn->show();

    // 1. 获取 AGV 当前位姿
    QPointF agvPos = w->m_agvLayer->getPos();
    double agvAngle = w->m_agvLayer->getAngle();

    // 2. 初始化重定位图层位置 (注意 Y 轴镜像)
    w->m_reloLayer->setPos(QPointF(agvPos.x(), -agvPos.y()));
    w->m_reloLayer->setAngle(agvAngle);

    // 3. 锁定点云到局部坐标系，以便随重定位图层旋转/平移
    w->m_pointCloudLayer->lockToLocal(agvPos, agvAngle);

    w->update();
}

void RelocationController::finish()
{
    // 获取重定位图层的当前位姿并发送信号 (反算回世界坐标系)
    emit w->baseIniPose(QPointF(w->m_reloLayer->pos().x(), -w->m_reloLayer->pos().y()), 
                        w->m_reloLayer->getAngle());
    exitMode();
}

void RelocationController::cancel()
{
    exitMode();
}

void RelocationController::exitMode()
{
    w->m_isRelocating = false;
    w->m_reloLayer->setVisible(false);
    w->m_pointCloudLayer->unlock();

    w->m_confirmBtn->hide();
    w->m_cancelBtn->hide();
    w->m_reloBtn->show();

    w->update();
}