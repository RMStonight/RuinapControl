#ifndef MANUALCONTROLWIDGET_H
#define MANUALCONTROLWIDGET_H

#include "BaseDisplayWidget.h"
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <AgvData.h>
#include "ConfigManager.h"

enum class ButtonType
{
    Move,   // 移动控制
    Act,    // 动作控制
    Cancel, // 取消任务
    Start,  // 开始任务
    Pause,  // 暂停任务
    Resume  // 恢复任务
};

class ManualControlWidget : public BaseDisplayWidget
{
    Q_OBJECT
public:
    explicit ManualControlWidget(QWidget *parent = nullptr);
    ~ManualControlWidget();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateUi();

private:
    void initUi();
    // 统一创建按住触发按钮的函数
    QPushButton *createMomentaryButton(ButtonType type, const QString &text, const QString &color, int val);

    QWidget *m_contentContainer;

    // agvData
    AgvData *agvData = AgvData::instance();

    // cfg
    ConfigManager *cfg = ConfigManager::instance();

    QCheckBox *chargeCheck; // 手动充电选择框

    // 定时器更新 UI
    QTimer *m_updateTimer;
    const int UPDATE_INTERVAL_MS = 5000;
};

#endif