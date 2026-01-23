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

    // ==========================================
    // Tab 2: 手动控制
    // ==========================================
    QWidget *manualTab = new QWidget();
    QVBoxLayout *manualLayout = new QVBoxLayout(manualTab);
    manualLayout->setContentsMargins(0, 20, 0, 0); // 去除左右下边距，上边距留一点

    // 添加中间的内容 (模拟 placeholder)
    QLabel *manualLabel = new QLabel("当前页面：手动控制", manualTab);
    manualLabel->setStyleSheet("font-size: 20px; color: #888888; font-weight: bold;");
    manualLabel->setAlignment(Qt::AlignCenter);

    manualLayout->addWidget(manualLabel);
    manualLayout->addStretch(); // 关键：将底部栏顶到底部

    m_tabWidget->addTab(manualTab, "手动控制");

    // ==========================================
    // Tab 3: 实时监控
    // ==========================================
    m_monitorTab = new MonitorWidget(this);
    m_tabWidget->addTab(m_monitorTab, "实时监控");

    // ==========================================
    // Tab 4 - 8: 其他功能页 (批量创建)
    // ==========================================
    // 定义剩下的8个标签名
    QStringList tabNames = {
        "任务管理", "详细状态",
        "可选信息", "日志记录", "用户权限"};

    for (const QString &name : tabNames)
    {
        // 调用辅助函数创建页面
        m_tabWidget->addTab(createPlaceholderTab(name), name);
    }

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
    bool needBottomBar = (currentTitle == "车辆信息" || currentTitle == "手动控制");

    if (needBottomBar) {
        m_bottomBar->show();
    } else {
        m_bottomBar->hide();
    }
}

// 更新数据的统一接口
void MainContentWidget::updateBottomBarData(const QString &key, const QString &value)
{
    if (m_bottomBar) {
        m_bottomBar->updateValue(key, value);
    }
}