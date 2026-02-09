#include "LogDisplayWidget.h"
#include <QScrollBar>

LogDisplayWidget::LogDisplayWidget(QWidget *parent) : BaseDisplayWidget(parent)
{
    this->setStyleSheet("background-color: #ffffff;");
    initUi();

    connect(&LogManager::instance(), &LogManager::newLogEntry, this, &LogDisplayWidget::handleNewLog);

    // 连接勾选框状态改变信号
    connect(m_infoCheck, &QCheckBox::toggled, this, &LogDisplayWidget::onFilterChanged);
    connect(m_warnCheck, &QCheckBox::toggled, this, &LogDisplayWidget::onFilterChanged);
    connect(m_errorCheck, &QCheckBox::toggled, this, &LogDisplayWidget::onFilterChanged);
}

void LogDisplayWidget::initUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(10);

    // 1. 日志显示区 (参考 SerialDebugWidget)
    m_logDisplay = new QPlainTextEdit();
    m_logDisplay->setReadOnly(true);
    m_logDisplay->setMaximumBlockCount(MAX_LOG_LINES);
    m_logDisplay->setLineWrapMode(QPlainTextEdit::WidgetWidth);

    // 设置深色/专业字体样式
    m_logDisplay->setStyleSheet(
        "QPlainTextEdit {"
        "  background-color: #F5F5F5;"
        "  border: 1px solid #DDD;"
        "  font-family: 'Consolas', 'Monaco', 'Courier New';"
        "  font-size: 13px;"
        "  line-height: 150%;"
        "}");

    layout->addWidget(m_logDisplay);

    // 2. 底部筛选栏
    QHBoxLayout *bottomLayout = new QHBoxLayout();

    m_infoCheck = new QCheckBox(QStringLiteral("信息 (Info)"));
    m_warnCheck = new QCheckBox(QStringLiteral("警告 (Warning)"));
    m_errorCheck = new QCheckBox(QStringLiteral("错误 (Error)"));

    // 默认全部勾选
    m_infoCheck->setChecked(true);
    m_warnCheck->setChecked(true);
    m_errorCheck->setChecked(true);

    // 建立级别字符串与控件的映射 (需匹配 spdlog 生成的 level 字符串)
    m_filterMap.insert("info", m_infoCheck);
    m_filterMap.insert("warning", m_warnCheck);
    m_filterMap.insert("error", m_errorCheck);

    bottomLayout->addWidget(m_infoCheck);
    bottomLayout->addWidget(m_warnCheck);
    bottomLayout->addWidget(m_errorCheck);
    bottomLayout->addStretch(); // 将按钮推向左侧

    // 锁定滚动栏
    m_lockScrollBtn = new QPushButton(QStringLiteral("锁定滚动"));
    m_lockScrollBtn->setCheckable(true);
    m_lockScrollBtn->setChecked(false); // 默认不“锁定”（即自动滚动）

    // 样式：可以使用不同的颜色区分状态
    m_lockScrollBtn->setStyleSheet("QPushButton:checked { background-color: #FFCDD2; }");

    bottomLayout->addWidget(m_lockScrollBtn);

    // 连接信号
    connect(m_lockScrollBtn, &QPushButton::toggled, [this](bool checked)
            {
    m_autoScroll = !checked;
    m_lockScrollBtn->setText(checked ? QStringLiteral("解锁滚动") : QStringLiteral("锁定滚动")); });

    layout->addLayout(bottomLayout);

    // 3.创建遮罩层（初始隐藏）
    m_loadingOverlay = new QWidget(this);
    m_loadingOverlay->setObjectName("loadingOverlay");
    // 半透明黑色背景
    m_loadingOverlay->setStyleSheet("background-color: rgba(0, 0, 0, 100); border-radius: 5px;");
    m_loadingOverlay->hide();

    // 在遮罩中心添加“正在处理”文字
    QVBoxLayout *overlayLayout = new QVBoxLayout(m_loadingOverlay);
    QLabel *loadingLabel = new QLabel(QStringLiteral("正在筛选日志..."), m_loadingOverlay);
    loadingLabel->setStyleSheet("color: white; font-weight: bold; font-size: 16px; background: transparent;");
    loadingLabel->setAlignment(Qt::AlignCenter);
    overlayLayout->addWidget(loadingLabel);
}

void LogDisplayWidget::handleNewLog(const QString &time, const QString &level, const QString &msg)
{
    QString lowerLevel = level.toLower();

    QString levelColor = "#666666";
    if (lowerLevel == "warning")
        levelColor = "#FF9800";
    else if (lowerLevel == "error")
        levelColor = "#F44336";
    else if (lowerLevel == "info")
        levelColor = "#4CAF50";

    QString htmlFragment = QString(
                               "<span style='color:#888888;'>[%1]</span> "
                               "<span style='color:%2; font-weight:bold;'>[%3]</span> "
                               "<span style='color:#333333;'>%4</span>")
                               .arg(time)
                               .arg(levelColor)
                               .arg(level.toUpper())
                               .arg(msg.toHtmlEscaped());

    // 存储到缓存（包含预格式化的内容）
    LogEntry entry = {time, level, msg, htmlFragment};
    m_logCache.append(entry);

    if (m_logCache.size() > MAX_LOG_LINES)
    {
        m_logCache.removeFirst();
    }

    // 直接追加到显示区（如果满足当前筛选）
    if (m_filterMap.contains(lowerLevel) && m_filterMap[lowerLevel]->isChecked())
    {
        m_logDisplay->appendHtml(htmlFragment);
        if (m_autoScroll)
        {
            m_logDisplay->verticalScrollBar()->setValue(m_logDisplay->verticalScrollBar()->maximum());
        }
    }
}

void LogDisplayWidget::onFilterChanged()
{
    // 1. 显示加载遮罩并禁用交互
    showLoading(true);

    // 2. 延迟 50ms 执行，确保遮罩层能先画出来
    QTimer::singleShot(50, this, [this]()
                       {
                           refreshDisplay();
                           showLoading(false); // 完成后隐藏
                       });
}

void LogDisplayWidget::showLoading(bool show)
{
    if (show)
    {
        // 让遮罩大小与日志显示区一致
        m_loadingOverlay->setGeometry(m_logDisplay->geometry());
        m_loadingOverlay->raise(); // 确保在最上层
        m_loadingOverlay->show();
        // 仅禁用交互组件，不禁用整个页面，防止导航栏失效
        m_infoCheck->setEnabled(false);
        m_warnCheck->setEnabled(false);
        m_errorCheck->setEnabled(false);
        m_lockScrollBtn->setEnabled(false);
    }
    else
    {
        m_loadingOverlay->hide();
        m_infoCheck->setEnabled(true);
        m_warnCheck->setEnabled(true);
        m_errorCheck->setEnabled(true);
        m_lockScrollBtn->setEnabled(true);
    }
}

void LogDisplayWidget::refreshDisplay()
{
    m_logDisplay->setUpdatesEnabled(false);

    QStringList htmlList;
    for (const auto &entry : m_logCache)
    {
        if (m_filterMap.contains(entry.level.toLower()) && m_filterMap[entry.level.toLower()]->isChecked())
        {
            htmlList << entry.formattedHtml; // 直接使用预存结果，极快！
        }
    }

    m_logDisplay->document()->setHtml(htmlList.join("<br>"));
    m_logDisplay->verticalScrollBar()->setValue(m_logDisplay->verticalScrollBar()->maximum());

    m_logDisplay->setUpdatesEnabled(true);
}