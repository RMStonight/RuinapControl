#ifndef MAINCONTENTWIDGET_H
#define MAINCONTENTWIDGET_H

#include <QWidget>
#include <QTabBar>
#include "MonitorWidget.h"
#include "BottomInfoBar.h"
#include "VehicleInfoWidget.h"
#include "IoWidget.h"
#include "TruckLoadingWidget.h"
#include "ManualControlWidget.h"
#include "SerialDebugWidget.h"
#include "LogDisplayWidget.h"
#include "LogManager.h"

// 前置声明
class QTabWidget;
class QPushButton;

class MainContentWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MainContentWidget(QWidget *parent = nullptr);

    // 提供一个公共接口给外部（如 MainWindow）用来更新数据
    // 这样外部不需要知道 m_bottomBar 的存在，符合封装原则
    void updateBottomBarData(const QString &key, const QString &value);

signals:
    /**
     * @brief 通知外部发送WS请求
     */
    void requestTruckSize();
    void getTruckSize(const QString &dateTime, int width, int depth);

private slots:
    // 处理 Tab 切换，控制底部栏显示/隐藏
    void onTabChanged(int index);

private:
    // 日志管理器
    LogManager *logger = &LogManager::instance();

    void initLayout();

    // 辅助函数：快速创建一个带有简单文字的空白页，用于填充 Tab
    QWidget *createPlaceholderTab(const QString &text);

    OptionalInfoWidget *m_sharedOptionalInfo; // 唯一的右侧栏实例
    QTabWidget *m_tabWidget;                  // Tab 容器
    QPushButton *m_testBtn;                   // 保留原来的按钮

    VehicleInfoWidget *m_vehicleInfoTab;     //  vehicleInfo 标签页
    ManualControlWidget *m_manualControlTab; //  manualControl 标签页
    MonitorWidget *m_monitorTab;             //  monitor 标签页
    IoWidget *m_ioTab;                       //  Io 标签页
    TruckLoadingWidget *m_truckLoadingTab;   //  装车管理 标签页
    SerialDebugWidget *m_serialTab;          //  串口调试标签页
    LogDisplayWidget *m_logTab;              //  日志记录标签页

    BottomInfoBar *m_bottomBar; // 底部栏
};

#endif // MAINCONTENTWIDGET_H