#ifndef SERIALDEBUGWIDGET_H
#define SERIALDEBUGWIDGET_H

#include "BaseDisplayWidget.h"
#include <QSerialPort>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include "ConfigManager.h"
#include "LogManager.h"
#include <QFile>
#include <QDir>
#include <QTextStream>

class SerialDebugWidget : public BaseDisplayWidget
{
    Q_OBJECT
public:
    explicit SerialDebugWidget(QWidget *parent = nullptr);
    ~SerialDebugWidget();

private slots:
    void toggleSerialPort();
    void readData();
    void saveLogToFile();
    void updatePermissionView();

protected:
    void showEvent(QShowEvent *event) override;

private:
    LogManager *logger = &LogManager::instance();   // 日志管理器
    ConfigManager *cfg = ConfigManager::instance(); // 系统参数

    QWidget *m_mainContainer;  // 原有的业务 UI 容器
    QWidget *m_permissionPage; // 权限提示容器

    void initUi();

    QByteArray m_buffer; // 增加一个缓冲区
    QSerialPort *m_serialPort;
    QPlainTextEdit *m_logDisplay;
    QPushButton *m_switchBtn;
    QPushButton *m_clearBtn;
    QPushButton *m_saveBtn;

    const int MAX_LINE_COUNT = 2000; // 行数限制
};

#endif // SERIALDEBUGWIDGET_H