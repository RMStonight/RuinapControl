#ifndef SERIALDEBUGWIDGET_H
#define SERIALDEBUGWIDGET_H

#include "BaseDisplayWidget.h"
#include <QSerialPort>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include "ConfigManager.h"

class SerialDebugWidget : public BaseDisplayWidget
{
    Q_OBJECT
public:
    explicit SerialDebugWidget(QWidget *parent = nullptr);
    ~SerialDebugWidget();

private slots:
    void toggleSerialPort();
    void readData();

private:
    void initUi();

    QByteArray m_buffer; // 增加一个缓冲区
    QSerialPort *m_serialPort;
    QPlainTextEdit *m_logDisplay;
    QPushButton *m_switchBtn;
    QPushButton *m_clearBtn;

    const int MAX_LINE_COUNT = 2000; // 行数限制
};

#endif // SERIALDEBUGWIDGET_H