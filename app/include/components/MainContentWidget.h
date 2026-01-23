#ifndef MAINCONTENTWIDGET_H
#define MAINCONTENTWIDGET_H

#include <QWidget>
#include <QTabBar>
#include "components/MonitorWidget.h"
#include "components/BottomInfoBar.h"
#include "components/VehicleInfoWidget.h"

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

private slots:
    // 处理 Tab 切换，控制底部栏显示/隐藏
    void onTabChanged(int index);

private:
    void initLayout();

    // 辅助函数：快速创建一个带有简单文字的空白页，用于填充 Tab
    QWidget *createPlaceholderTab(const QString &text);

    QTabWidget *m_tabWidget; // Tab 容器
    QPushButton *m_testBtn;  // 保留原来的按钮

    VehicleInfoWidget *m_vehicleInfoTab;    //  vehicleInfo 标签页
    MonitorWidget *m_monitorTab;    //  monitor 标签页

    BottomInfoBar *m_bottomBar;     // 底部栏
};

#endif // MAINCONTENTWIDGET_H