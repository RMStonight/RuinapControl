#include "components/MainContentWidget.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "components/SystemSettingsWidget.h"

MainContentWidget::MainContentWidget(QWidget *parent) : QWidget(parent)
{
    // 设置整体背景色
    this->setStyleSheet("background-color: #FFFFFF;");

    m_sharedOptionalInfo = new OptionalInfoWidget(this);

    // 初始化布局
    initLayout();
}

void MainContentWidget::initLayout()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 创建 TabWidget
    m_tabWidget = new QTabWidget(this);

    // ===============================================
    // 从 QSS 文件加载
    // ===============================================
    QFile file(":/styles/main_content.qss"); // 冒号开头表示从资源读取
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream stream(&file);
        m_tabWidget->setStyleSheet(stream.readAll());
        file.close();
    }
    else
    {
        qWarning() << "Error: Failed to load stylesheet from :/styles/main_content.qss";
    }

    // 设置 Tab 扩展属性 (触控屏友好)
    QTabBar *tabBar = m_tabWidget->tabBar();
    tabBar->setDocumentMode(true);
    tabBar->setExpanding(true);

    // ==========================================
    // Tab 1: 车辆信息
    // ==========================================
    m_vehicleInfoTab = new VehicleInfoWidget(this);
    m_tabWidget->addTab(m_vehicleInfoTab, "车辆信息");
    m_vehicleInfoTab->setSharedOptionalInfo(m_sharedOptionalInfo);

    // ==========================================
    // Tab 2: 手动控制
    // ==========================================
    m_manualControlTab = new ManualControlWidget(this);
    m_tabWidget->addTab(m_manualControlTab, "手动控制");

    // ==========================================
    // Tab 3: 实时监控
    // ==========================================
    m_monitorTab = new MonitorWidget(this);
    m_tabWidget->addTab(m_monitorTab, "实时监控");

    // ==========================================
    // Tab 4: 任务管理
    // ==========================================
    m_tabWidget->addTab(createPlaceholderTab("任务管理"), "任务管理");

    // ==========================================
    // Tab 5: IO信号
    // ==========================================
    m_ioTab = new IoWidget(this);
    m_tabWidget->addTab(m_ioTab, "IO信号");

    // ==========================================
    // Tab 6 - 8: 其他功能页
    // ==========================================

    m_tabWidget->addTab(createPlaceholderTab("调试专用"), "调试专用");
    m_tabWidget->addTab(createPlaceholderTab("日志记录"), "日志记录");
    m_tabWidget->addTab(createPlaceholderTab("用户权限"), "用户权限");
    // ==========================================
    // Tab 9: 系统设置页面
    // ==========================================
    SystemSettingsWidget *settingsTab = new SystemSettingsWidget(this);
    m_tabWidget->addTab(settingsTab, "系统设置"); // 将其实例化并加入 Tab

    // 将 TabWidget 加入主布局
    mainLayout->addWidget(m_tabWidget);

    // 创建全局唯一的 BottomInfoBar
    m_bottomBar = new BottomInfoBar(this);
    mainLayout->addWidget(m_bottomBar, 0);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainContentWidget::onTabChanged);
    onTabChanged(m_tabWidget->currentIndex());

    // 特殊处理信号，mapId 变化
    connect(m_bottomBar, &BottomInfoBar::mapIdChanged, m_monitorTab, &MonitorWidget::handleMapIdChanged);
}

// 辅助函数：创建一个只显示一行字的 Widget
QWidget *MainContentWidget::createPlaceholderTab(const QString &text)
{
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);

    QLabel *label = new QLabel(QStringLiteral("当前页面：") + text, tab);
    label->setStyleSheet("font-size: 20px; color: #888888; font-weight: bold;");
    label->setAlignment(Qt::AlignCenter);

    layout->addWidget(label);
    return tab;
}

// Tab 切换槽函数
void MainContentWidget::onTabChanged(int index)
{
    QString currentTitle = m_tabWidget->tabText(index);

    // 定义哪些页面需要显示底部栏
    // 你也可以用 index 判断，但用标题更直观，或者维护一个 index 列表
    bool noBottomBar = (currentTitle == "日志记录" || currentTitle == "用户权限" || currentTitle == "系统设置");
    m_bottomBar->setVisible(!noBottomBar);

    // 侧边栏逻辑简化
    bool hasSideBar = (currentTitle == "车辆信息" || currentTitle == "手动控制" || currentTitle == "实时监控" || currentTitle == "IO信号");
    if (m_sharedOptionalInfo)
    {
        m_sharedOptionalInfo->setVisible(hasSideBar);

        // 关键：触发当前显示的 Widget 重新计算布局
        // 这样当切换回显示页面时，基类的 resizeEvent 或手动调用会确保位置正确
        if (hasSideBar)
        {
            // 获取当前活跃的标签页并强制其刷新侧边栏位置
            BaseDisplayWidget *currentWidget = qobject_cast<BaseDisplayWidget *>(m_tabWidget->currentWidget());
            if (currentWidget)
            {
                currentWidget->setSharedOptionalInfo(m_sharedOptionalInfo);
            }
        }
    }
}

// 更新数据的统一接口
void MainContentWidget::updateBottomBarData(const QString &key, const QString &value)
{
    if (m_bottomBar)
    {
        m_bottomBar->updateValue(key, value);
    }
}