#include "components/MainContentWidget.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include <QPushButton>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "components/SystemSettingsWidget.h"
#include "components/MonitorWidget.h"

MainContentWidget::MainContentWidget(QWidget *parent) : QWidget(parent)
{
    // 设置整体背景色
    this->setStyleSheet("background-color: #FFFFFF;");

    // 初始化布局
    initLayout();

    // 连接信号（逻辑不变）
    // 虽然按钮现在是在 Tab 里面，但信号依然从 MainContentWidget 发出
    // 外部 MainWindow 不需要知道按钮藏得有多深
    connect(m_testBtn, &QPushButton::clicked, this, &MainContentWidget::testBtnClicked);
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
    // Tab 1: 系统概览 (保留原来的测试按钮)
    // ==========================================
    QWidget *tabOverview = new QWidget();
    QVBoxLayout *overviewLayout = new QVBoxLayout(tabOverview);

    m_testBtn = new QPushButton("点击测试 Header 更新", tabOverview);
    m_testBtn->setFixedSize(180, 50);

    overviewLayout->addStretch();
    overviewLayout->addWidget(new QLabel("这里是系统概览页面", tabOverview), 0, Qt::AlignCenter);
    overviewLayout->addWidget(m_testBtn, 0, Qt::AlignCenter);
    overviewLayout->addStretch();

    m_tabWidget->addTab(tabOverview, "系统概览");

    // ==========================================
    // Tab 2: 实时监控 (不再是占位符)
    // ==========================================
    MonitorWidget *monitorTab = new MonitorWidget(this);
    m_tabWidget->addTab(monitorTab, "实时监控");

    // ==========================================
    // Tab 3 - 9: 其他功能页 (批量创建)
    // ==========================================
    // 定义剩下的8个标签名
    QStringList tabNames = {
        "任务管理", "地图编辑", "车辆状态",
        "报警记录", "日志分析", "用户权限"};

    for (const QString &name : tabNames)
    {
        // 调用辅助函数创建页面
        m_tabWidget->addTab(createPlaceholderTab(name), name);
    }

    // ==========================================
    // 特殊处理：系统设置页面
    // ==========================================
    SystemSettingsWidget *settingsTab = new SystemSettingsWidget(this);
    m_tabWidget->addTab(settingsTab, "系统设置"); // 将其实例化并加入 Tab

    // 将 TabWidget 加入主布局
    mainLayout->addWidget(m_tabWidget);
}

// 辅助函数：创建一个只显示一行字的 Widget
QWidget *MainContentWidget::createPlaceholderTab(const QString &text)
{
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);

    QLabel *label = new QLabel(QString("当前页面：") + text, tab);
    label->setStyleSheet("font-size: 20px; color: #888888; font-weight: bold;");
    label->setAlignment(Qt::AlignCenter);

    layout->addWidget(label);
    return tab;
}