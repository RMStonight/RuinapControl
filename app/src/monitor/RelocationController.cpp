#include "monitor/RelocationController.h"
#include "components/MonitorWidget.h"
#include "layers/AgvLayer.h"
#include "layers/RelocationLayer.h"
#include "layers/PointCloudLayer.h"
#include "layers/FixedRelocationLayer.h"
#include <QPushButton>

RelocationController::RelocationController(MonitorWidget *parent)
    : QObject(parent), w(parent) {}

void RelocationController::start()
{
    if (m_currentMode == FixedMode)
    {
        // qDebug() << "Entering [Fixed Relocation] Mode: Execution logic to be implemented...";
        // 暂时只打印，不进入常规重定位流程
        return;
    }

    // 自由重定位逻辑
    w->m_isRelocating = true;
    w->m_reloLayer->setVisible(true);

    // 控制按钮显隐
    w->m_reloBtn->hide();
    w->m_switchBtn->hide();
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

void RelocationController::switchMode()
{
    // 切换状态
    m_currentMode = (m_currentMode == FreeMode) ? FixedMode : FreeMode;

    // 更新 MonitorWidget 的按钮 UI
    if (w)
    {
        setMode(m_currentMode);
    }
}

void RelocationController::setMode(ReloMode mode)
{
    m_currentMode = mode;
    if (mode == FreeMode)
    {
        w->m_reloBtn->setText("自由重定位");
        w->m_reloBtn->setStyleSheet("QPushButton { border-radius: 5px; font-weight: bold; color: white; background-color: #0078d7; }");
        w->m_reloBtn->setEnabled(true);
        if (w->m_fixedReloLayer)
            w->m_fixedReloLayer->setVisible(false);
    }
    else
    {
        w->m_reloBtn->setText("固定重定位");
        w->m_reloBtn->setStyleSheet("QPushButton { border-radius: 5px; font-weight: bold; color: white; background-color: #ccc; }");
        w->m_reloBtn->setEnabled(false);
        if (w->m_fixedReloLayer)
            w->m_fixedReloLayer->setVisible(true);
    }
}

void RelocationController::exitMode()
{
    w->m_isRelocating = false;
    w->m_reloLayer->setVisible(false);
    w->m_pointCloudLayer->unlock();

    w->m_confirmBtn->hide();
    w->m_cancelBtn->hide();
    w->m_reloBtn->show();
    w->m_switchBtn->show();

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