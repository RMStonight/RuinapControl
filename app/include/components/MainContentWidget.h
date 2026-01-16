#ifndef MAINCONTENTWIDGET_H
#define MAINCONTENTWIDGET_H

#include <QWidget>
#include <QTabBar>

// 前置声明
class QTabWidget;
class QPushButton;

class MainContentWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MainContentWidget(QWidget *parent = nullptr);

signals:
    // 保持原有信号不变，这样 MainWindow 不需要改代码
    void testBtnClicked();

private:
    void initLayout();
    
    // 辅助函数：快速创建一个带有简单文字的空白页，用于填充 Tab
    QWidget* createPlaceholderTab(const QString &text);

    QTabWidget *m_tabWidget; // Tab 容器
    QPushButton *m_testBtn;  // 保留原来的按钮
};

#endif // MAINCONTENTWIDGET_H