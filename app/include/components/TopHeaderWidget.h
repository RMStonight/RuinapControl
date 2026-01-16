// include/components/TopHeaderWidget.h
#ifndef TOPHEADERWIDGET_H
#define TOPHEADERWIDGET_H

#include <QWidget>

// 左上角 logo 的命名方式
#define TOP_LEFT_LOGO "top_left_logo.png"

class QLabel;
class QProgressBar;

class TopHeaderWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TopHeaderWidget(QWidget *parent = nullptr);

    // --- 公开接口：供主窗口调用以更新显示 ---
    void setAgvInfo(const QString &id, const QString &ip);
    void setBatteryLevel(int level);
    void setRunMode(const QString &mode);
    void setTaskStatus(const QString &status);

private slots:
    void updateInfoFromConfig(); // 新增槽函数

private:
    void initLayout(); // 内部初始化布局


    // UI 控件指针
    QLabel *m_logoLabel;
    QLabel *m_agvIdLabel;
    QLabel *m_ipLabel;
    QLabel *m_runModeLabel;
    QLabel *m_taskStatusLabel;
    QProgressBar *m_batteryBar;
};

#endif // TOPHEADERWIDGET_H