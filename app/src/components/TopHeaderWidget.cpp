// src/components/TopHeaderWidget.cpp
#include "components/TopHeaderWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPixmap>
#include "utils/ConfigManager.h"

TopHeaderWidget::TopHeaderWidget(QWidget *parent) : QWidget(parent)
{
    // 设置自身属性
    this->setFixedHeight(100);
    this->setObjectName("headerContainer");
    // 将样式表移到这里，实现“高内聚”
    this->setStyleSheet("#headerContainer { border-bottom: 1px solid #A0A0A0; }");
    // this->setStyleSheet("#headerContainer { background-color: #E0E0E0; border-bottom: 1px solid #A0A0A0; }");

    initLayout();

    // 【新增】初始化时，直接读取配置显示
    updateInfoFromConfig();
    // 【新增】监听配置修改信号
    // 当 SystemSettingsWidget 保存后，ConfigManager 发出信号，这里自动刷新
    connect(ConfigManager::instance(), &ConfigManager::configChanged,
            this, &TopHeaderWidget::updateInfoFromConfig);
}

void TopHeaderWidget::initLayout()
{
    QHBoxLayout *topLayout = new QHBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(0);

    // ==========================================
    // 【左侧容器】
    // ==========================================
    QWidget *leftContainer = new QWidget(this);
    leftContainer->setFixedWidth(280);

    QHBoxLayout *leftLayout = new QHBoxLayout(leftContainer);
    leftLayout->setContentsMargins(20, 0, 0, 0);
    leftLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // 改为从内存单例读取
    ConfigManager *cfg = ConfigManager::instance();
    m_logoLabel = new QLabel(this);
    QString resourceFolder = cfg->resourceFolder();
    // 如果相对路径不是以 / 结尾则需要添加
    if (!resourceFolder.endsWith("/"))
    {
        resourceFolder += "/";
    }

    QPixmap logo(resourceFolder + TOP_LEFT_LOGO);
    if (!logo.isNull())
    {
        m_logoLabel->setPixmap(logo.scaled(200, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    else
    {
        m_logoLabel->setText("Logo");
    }
    leftLayout->addWidget(m_logoLabel);

    // ==========================================
    // 【中间容器】
    // ==========================================
    QWidget *centerContainer = new QWidget(this);
    centerContainer->setStyleSheet("background-color: transparent;");

    QVBoxLayout *centerLayout = new QVBoxLayout(centerContainer);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(5);
    centerLayout->setAlignment(Qt::AlignCenter);

    m_agvIdLabel = new QLabel(this);
    m_ipLabel = new QLabel(this);

    // 初始化默认值
    setAgvInfo("1", "192.168.160.231");

    // 设置字体
    QFont fontId;
    fontId.setBold(true);
    fontId.setPointSize(25);
    m_agvIdLabel->setFont(fontId);

    QFont fontIp;
    fontIp.setBold(true);
    fontIp.setPointSize(12);
    m_ipLabel->setFont(fontIp);

    m_agvIdLabel->setAlignment(Qt::AlignCenter);
    m_ipLabel->setAlignment(Qt::AlignCenter);

    centerLayout->addWidget(m_agvIdLabel);
    centerLayout->addWidget(m_ipLabel);

    // ==========================================
    // 【右侧容器】
    // ==========================================
    QWidget *rightContainer = new QWidget(this);
    rightContainer->setFixedWidth(280);

    QVBoxLayout *rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 10, 20, 10);
    rightLayout->setSpacing(5);
    rightLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_runModeLabel = new QLabel("运行模式：自动导航", this);
    m_taskStatusLabel = new QLabel("当前任务：物料搬运", this);

    // 设置右侧字体
    QFont rightFont;
    rightFont.setPointSize(10);
    rightFont.setBold(true);
    m_runModeLabel->setFont(rightFont);
    m_taskStatusLabel->setFont(rightFont);
    m_runModeLabel->setAlignment(Qt::AlignRight);
    m_taskStatusLabel->setAlignment(Qt::AlignRight);

    // 电池条
    m_batteryBar = new QProgressBar(this);
    m_batteryBar->setRange(0, 100);
    m_batteryBar->setFixedWidth(120);
    m_batteryBar->setFixedHeight(16);
    // 样式表保持不变
    m_batteryBar->setStyleSheet(
        "QProgressBar { border: 1px solid #999999; border-radius: 8px; background-color: #FFFFFF; text-align: center; }"
        "QProgressBar::chunk { background-color: #00CD00; border-radius: 7px; }");
    setBatteryLevel(85); // 默认值

    rightLayout->addWidget(m_runModeLabel);
    rightLayout->addWidget(m_taskStatusLabel);
    rightLayout->addWidget(m_batteryBar);

    // ==========================================
    // 加入主布局
    // ==========================================
    topLayout->addWidget(leftContainer);
    topLayout->addWidget(centerContainer);
    topLayout->addWidget(rightContainer);
}

// 私有槽函数
void TopHeaderWidget::updateInfoFromConfig()
{
    ConfigManager *cfg = ConfigManager::instance();
    // 使用读取到的配置更新 UI
    setAgvInfo(cfg->agvId(), cfg->agvIp());
}

// 实现公开接口
void TopHeaderWidget::setAgvInfo(const QString &id, const QString &ip)
{
    m_agvIdLabel->setText(QString("AGV编号：<span style='color: #016f56;'>%1</span>").arg(id));
    m_ipLabel->setText(QString("IP：<span style='color: #016f56;'>%1</span>").arg(ip));
}

void TopHeaderWidget::setBatteryLevel(int level)
{
    m_batteryBar->setValue(level);
}

void TopHeaderWidget::setRunMode(const QString &mode)
{
    m_runModeLabel->setText("运行模式：" + mode);
}

void TopHeaderWidget::setTaskStatus(const QString &status)
{
    m_taskStatusLabel->setText("当前任务：" + status);
}