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

    // 1. 顶部筛选栏
    QHBoxLayout *topLayout = new QHBoxLayout();

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

    topLayout->addWidget(m_infoCheck);
    topLayout->addWidget(m_warnCheck);
    topLayout->addWidget(m_errorCheck);
    topLayout->addStretch(); // 将按钮推向左侧

    layout->addLayout(topLayout);

    // 2. 日志显示区 (参考 SerialDebugWidget)
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
}

void LogDisplayWidget::handleNewLog(const QString &time, const QString &level, const QString &msg)
{
    // 1. 维护缓存：永远保留最新的 2000 条数据
    LogEntry entry = {time, level, msg};
    m_logCache.append(entry);

    if (m_logCache.size() > MAX_LOG_LINES)
    {
        m_logCache.removeFirst();
    }

    // 2. 检查当前新日志是否满足筛选条件，满足则直接追加（优化性能，避免全量刷新）
    QString lowerLevel = level.toLower();
    if (m_filterMap.contains(lowerLevel) && m_filterMap[lowerLevel]->isChecked())
    {

        QString levelColor = "#666666";
        if (lowerLevel == "warning")
            levelColor = "#FF9800";
        else if (lowerLevel == "error")
            levelColor = "#F44336";
        else if (lowerLevel == "info")
            levelColor = "#4CAF50";

        QString formattedLog = QString(
                                   "<span style='color:#888888;'>[%1]</span> "
                                   "<span style='color:%2; font-weight:bold;'>[%3]</span> "
                                   "<span style='color:#333333;'>%4</span>")
                                   .arg(time)
                                   .arg(levelColor)
                                   .arg(level.toUpper())
                                   .arg(msg.toHtmlEscaped());

        m_logDisplay->appendHtml(formattedLog);
        m_logDisplay->verticalScrollBar()->setValue(m_logDisplay->verticalScrollBar()->maximum());
    }
}

void LogDisplayWidget::onFilterChanged()
{
    // 当筛选条件改变时，需要根据缓存重新生成显示内容
    refreshDisplay();
}

void LogDisplayWidget::refreshDisplay()
{
    m_logDisplay->clear();

    // 阻止 QPlainTextEdit 在批量插入时频繁重绘，提升性能
    m_logDisplay->setUpdatesEnabled(false);

    for (const auto &entry : m_logCache)
    {
        QString lowerLevel = entry.level.toLower();

        // 筛选逻辑
        if (m_filterMap.contains(lowerLevel) && !m_filterMap[lowerLevel]->isChecked())
        {
            continue;
        }

        QString levelColor = "#666666";
        if (lowerLevel == "warning")
            levelColor = "#FF9800";
        else if (lowerLevel == "error")
            levelColor = "#F44336";
        else if (lowerLevel == "info")
            levelColor = "#4CAF50";

        QString formattedLog = QString(
                                   "<span style='color:#888888;'>[%1]</span> "
                                   "<span style='color:%2; font-weight:bold;'>[%3]</span> "
                                   "<span style='color:#333333;'>%4</span>")
                                   .arg(entry.time)
                                   .arg(levelColor)
                                   .arg(entry.level.toUpper())
                                   .arg(entry.msg.toHtmlEscaped());

        m_logDisplay->appendHtml(formattedLog);
    }

    m_logDisplay->setUpdatesEnabled(true);
    m_logDisplay->verticalScrollBar()->setValue(m_logDisplay->verticalScrollBar()->maximum());
}