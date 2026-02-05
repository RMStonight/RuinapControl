#ifndef LOGDISPLAYWIDGET_H
#define LOGDISPLAYWIDGET_H

#include "BaseDisplayWidget.h"
#include "LogManager.h"
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QList>

// 定义单条日志结构体
struct LogEntry {
    QString time;
    QString level;
    QString msg;
};

class LogDisplayWidget : public BaseDisplayWidget
{
    Q_OBJECT
public:
    explicit LogDisplayWidget(QWidget *parent = nullptr);

private slots:
    void handleNewLog(const QString &time, const QString &level, const QString &msg);
    void onFilterChanged(); // 响应勾选状态变化

private:
    void initUi();
    void refreshDisplay();  // 根据当前缓存和筛选逻辑刷新 UI

    QPlainTextEdit *m_logDisplay;
    QCheckBox *m_infoCheck;
    QCheckBox *m_warnCheck;
    QCheckBox *m_errorCheck;

    QMap<QString, QCheckBox*> m_filterMap;
    
    // 日志缓存列表
    QList<LogEntry> m_logCache;
    const int MAX_LOG_LINES = 2000; 
};

#endif