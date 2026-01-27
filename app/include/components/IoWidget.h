#ifndef IOWIDGET_H
#define IOWIDGET_H

#include "BaseDisplayWidget.h"
#include <QWidget>
#include <QLabel>
#include <QVector>
#include <QTimer>

class IoWidget : public BaseDisplayWidget
{
    Q_OBJECT
public:
    explicit IoWidget(QWidget *parent = nullptr);
    ~IoWidget();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // 定时刷新槽函数
    void updateIoStatus();

private:
    // 初始化界面布局
    void initUi();

    // 辅助函数：创建一个圆形的指示灯Label
    QLabel *createLamp(const QString &text);

    // 辅助函数：设置灯的状态（绿色=触发，红色=未触发）
    void setLampState(QLabel *lamp, bool isOn);

    // 存储所有的信号灯指针，方便按索引访问
    QVector<QLabel *> m_xLamps; // 对应 X1-X24
    QVector<QLabel *> m_yLamps; // 对应 Y1-Y24

    QTimer *m_updateTimer;
    const int UPDATE_INTERVAL_MS = 100;

    QWidget *m_scrollContainer;
};

#endif // IOWIDGET_H