#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "components/TopHeaderWidget.h"
#include "components/MainContentWidget.h"
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QPixmap>
#include <QDebug>
#include <QProgressBar>
#include "CommunicationWsClient.h"

// 前置声明：告诉编译器有这些类，但先不包含具体头文件，能加快编译速度
class QLabel;
class QPushButton;
class QProgressBar; // 前置声明

class MainWindow : public QMainWindow
{
Q_OBJECT // 必须加这个宏，否则信号槽无法工作

    public : explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    // 通讯模块
    CommunicationWsClient *m_commClient;

    // --- 成员变量（UI控件） ---
    // 把控件定义为成员变量，这样你在类的任何函数里都能控制它们
    TopHeaderWidget *m_topHeader;
    MainContentWidget *m_mainContent;

    // --- 私有函数 ---
    void initUI();           // 专门用来初始化界面的辅助函数
    void setupConnections(); // 专门用来连接信号槽的辅助函数
    void applyWindowState();
};

#endif // MAINWINDOW_H