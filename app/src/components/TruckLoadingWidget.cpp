#include "TruckLoadingWidget.h"

TruckLoadingWidget::TruckLoadingWidget(QWidget *parent) : BaseDisplayWidget(parent)
{
    // 保持与 LogDisplayWidget 一致的底色
    this->setStyleSheet("background-color: #ffffff;");

    // 初始化定时器
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true); // 设置为一次性定时器

    initUi();

    // 连接超时处理逻辑
    connect(m_timeoutTimer, &QTimer::timeout, this, &TruckLoadingWidget::handleGetSizeTimeout);
}

void TruckLoadingWidget::initUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(10);

    // --- 1. 顶部操作栏 ---
    QHBoxLayout *topBarLayout = new QHBoxLayout();
    topBarLayout->setSpacing(10);

    // 获取尺寸按钮
    m_getSizeBtn = new QPushButton(QStringLiteral("获取车厢尺寸"));
    m_getSizeBtn->setFixedSize(120, 35);
    m_getSizeBtn->setStyleSheet(btn_unlock);

    // 时间戳显示
    m_timeLabel = new QLabel(QStringLiteral("更新时间: <span style='color: #F44336;'>--</span>"));
    m_timeLabel->setStyleSheet(LABEL_STYLE);

    // 宽度显示
    m_widthLabel = new QLabel(QStringLiteral("车厢宽度: <span style='color: #4CAF50;'>-- mm</span>"));
    m_widthLabel->setStyleSheet(LABEL_STYLE);

    // 深度显示
    m_depthLabel = new QLabel(QStringLiteral("车厢深度: <span style='color: #4CAF50;'>-- mm</span>"));
    m_depthLabel->setStyleSheet(LABEL_STYLE);

    // 组装顶部栏
    topBarLayout->addWidget(m_getSizeBtn);
    topBarLayout->addWidget(m_timeLabel);
    topBarLayout->addWidget(m_widthLabel);
    topBarLayout->addWidget(m_depthLabel);
    topBarLayout->addStretch(); // 顶部的弹簧，让内容靠左

    mainLayout->addLayout(topBarLayout);

    // --- 2. 下方占位弹簧 ---
    // 目前下方为空，添加一个垂直弹簧将顶部内容顶上去
    mainLayout->addStretch();

    // --- 信号连接 ---
    connect(m_getSizeBtn, &QPushButton::clicked, this, &TruckLoadingWidget::handleGetSizeBtn);
}

void TruckLoadingWidget::handleGetSizeBtn()
{
    // 设置状态
    m_isWaitingData = true;

    // 触发信号，告知外部逻辑需要发送WS请求
    emit requestTruckSize();

    // 可选：点击后临时禁用按钮防止连续点击
    m_getSizeBtn->setEnabled(false);
    m_getSizeBtn->setStyleSheet(btn_lock);
    m_timeLabel->setText(QStringLiteral("正在获取..."));

    // 启动3秒倒计时
    m_timeoutTimer->start(3000);
}

void TruckLoadingWidget::updateTruckData(const QString &timestamp, int width, int depth)
{
    // 如果当前不在等待状态（已超时或未点击），则直接忽略
    if (!m_isWaitingData)
    {
        return;
    }

    // 收到数据，停止定时器并重置状态
    m_timeoutTimer->stop();
    m_isWaitingData = false;

    // 时间戳显示为红色 (#F44336)
    m_timeLabel->setText(QString("更新时间: <span style='color: #F44336;'>%1</span>").arg(timestamp));

    // 车厢宽度：标签黑色，数值显示为绿色 (#4CAF50)
    m_widthLabel->setText(QString("车厢宽度: <span style='color: #4CAF50;'>%1 mm</span>").arg(width));

    // 车厢深度：标签黑色，数值显示为绿色 (#4CAF50)
    m_depthLabel->setText(QString("车厢深度: <span style='color: #4CAF50;'>%1 mm</span>").arg(depth));

    // 恢复按钮状态
    m_getSizeBtn->setEnabled(true);
    m_getSizeBtn->setStyleSheet(btn_unlock);
}

void TruckLoadingWidget::handleGetSizeTimeout()
{
    if (m_isWaitingData)
    {
        m_isWaitingData = false; // 标记不再等待，后续数据将被忽略
        m_timeLabel->setText(QStringLiteral("<span style='color: #F44336;'>检测算法超时</span>"));

        // 宽度显示
        m_widthLabel->setText(QStringLiteral("车厢宽度: <span style='color: #4CAF50;'>-- mm</span>"));

        // 深度显示
        m_depthLabel->setText(QStringLiteral("车厢深度: <span style='color: #4CAF50;'>-- mm</span>"));

        // 恢复按钮状态
        m_getSizeBtn->setEnabled(true);
        m_getSizeBtn->setStyleSheet(btn_unlock);
    }
}