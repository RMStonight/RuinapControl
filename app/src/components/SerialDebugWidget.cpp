#include "SerialDebugWidget.h"
#include <QScrollBar>
#include <QDateTime>

SerialDebugWidget::SerialDebugWidget(QWidget *parent) : BaseDisplayWidget(parent)
{
    this->setStyleSheet("background-color: #ffffff;");
    m_serialPort = new QSerialPort(this);

    initUi();

    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialDebugWidget::readData);
}

SerialDebugWidget::~SerialDebugWidget()
{
    if (m_serialPort->isOpen())
    {
        m_serialPort->close();
    }
}

void SerialDebugWidget::initUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(10);

    // 1. 顶部控制栏
    QHBoxLayout *topLayout = new QHBoxLayout();

    // 打开/关闭按钮
    m_switchBtn = new QPushButton("打开串口");
    m_switchBtn->setFixedSize(100, 40);
    m_switchBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; border-radius: 4px; font-weight: bold; }"
                               "QPushButton:checked { background-color: #F44336; }");
    m_switchBtn->setCheckable(true);
    topLayout->addWidget(m_switchBtn);

    // 弹簧，将后面的按钮推向最右侧
    topLayout->addStretch();

    // 保存按钮
    m_saveBtn = new QPushButton("保存");
    m_saveBtn->setFixedSize(100, 40);
    m_saveBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #2196F3;" // 蓝色
        "  color: white;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #1E88E5; }"
        "QPushButton:pressed { background-color: #1565C0; }");
    topLayout->addWidget(m_saveBtn);

    // 弹簧，将后面的按钮推向最右侧
    topLayout->addStretch();

    // 清空按钮
    m_clearBtn = new QPushButton("清空");
    m_clearBtn->setFixedSize(100, 40);
    m_clearBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #FF9800;" // 橙色
        "  color: white;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #FB8C00;" // 悬停略深
        "}"
        "QPushButton:pressed {"
        "  background-color: #E65100;" // 按下最深
        "}");
    topLayout->addWidget(m_clearBtn);

    layout->addLayout(topLayout);

    // 2. 接收显示区
    m_logDisplay = new QPlainTextEdit();
    m_logDisplay->setReadOnly(true);
    // m_logDisplay->setLineWrapMode(QPlainTextEdit::NoWrap); // 禁用自动换行，只有遇到 \n 或 \r 时才换行
    m_logDisplay->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_logDisplay->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_logDisplay->setMaximumBlockCount(MAX_LINE_COUNT); // 核心：限制行数，超过会自动删除旧行
    m_logDisplay->setStyleSheet("QPlainTextEdit { background-color: #F5F5F5; border: 1px solid #DDD; font-family: 'Consolas'; font-size: 13px; }");

    layout->addWidget(m_logDisplay);

    connect(m_switchBtn, &QPushButton::clicked, this, &SerialDebugWidget::toggleSerialPort);
    connect(m_clearBtn, &QPushButton::clicked, m_logDisplay, &QPlainTextEdit::clear);
    connect(m_saveBtn, &QPushButton::clicked, this, &SerialDebugWidget::saveLogToFile);
}

void SerialDebugWidget::toggleSerialPort()
{
    if (!m_serialPort->isOpen())
    {
        // 从 ConfigManager 读取配置
        m_serialPort->setPortName(cfg->microControllerCom());
        m_serialPort->setBaudRate(cfg->microControllerComBaudrate());
        m_serialPort->setDataBits(QSerialPort::Data8);
        m_serialPort->setParity(QSerialPort::NoParity);
        m_serialPort->setStopBits(QSerialPort::OneStop);
        m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

        if (m_serialPort->open(QIODevice::ReadOnly))
        {
            m_switchBtn->setText("关闭串口");
            m_switchBtn->setChecked(true);
            m_logDisplay->appendPlainText(tr("[%1] [%2] 串口已打开").arg(cfg->microControllerCom()).arg(cfg->microControllerComBaudrate()));
            logger->log(QStringLiteral("SerialDebugWidget"), spdlog::level::warn, QStringLiteral("[%1] [%2] 串口已打开").arg(cfg->microControllerCom()).arg(cfg->microControllerComBaudrate()));
        }
        else
        {
            m_switchBtn->setChecked(false);
            m_logDisplay->appendPlainText(tr("错误：[%1] [%2] 无法打开串口").arg(cfg->microControllerCom()).arg(cfg->microControllerComBaudrate()));
            logger->log(QStringLiteral("SerialDebugWidget"), spdlog::level::err, QStringLiteral("错误：[%1] [%2] 无法打开串口").arg(cfg->microControllerCom()).arg(cfg->microControllerComBaudrate()));
        }
    }
    else
    {
        m_serialPort->close();
        m_switchBtn->setText("打开串口");
        m_switchBtn->setChecked(false);
        m_logDisplay->appendPlainText("串口已关闭");
        logger->log(QStringLiteral("SerialDebugWidget"), spdlog::level::warn, QStringLiteral("串口已关闭"));
    }
}

void SerialDebugWidget::readData()
{
    // 将新读到的数据追加到缓冲区
    m_buffer.append(m_serialPort->readAll());

    // 安全检查：如果缓冲区超过 1MB 仍未找到换行符，可能是异常乱码或协议不匹配
    if (m_buffer.size() > 1024 * 1024)
    {
        m_buffer.clear();
        m_logDisplay->appendPlainText("警告：缓冲区过大，已强制清理");
        logger->log(QStringLiteral("SerialDebugWidget"), spdlog::level::warn, QStringLiteral("缓冲区过大，已强制清理"));
        return;
    }

    // 循环处理缓冲区，直到没有换行符为止
    while (m_buffer.contains('\n'))
    {
        int index = m_buffer.indexOf('\n');

        // 提取出一行完整数据（包含 \n）
        QByteArray lineData = m_buffer.left(index + 1);
        m_buffer.remove(0, index + 1);

        // 转换为字符串并清理掉末尾的空白符（\r \n 等）
        QString lineStr = QString::fromUtf8(lineData).trimmed();

        if (!lineStr.isEmpty())
        {
            // 获取当前时间，仅保留分:秒.毫秒 (精简版)
            QString timestamp = QDateTime::currentDateTime().toString("mm:ss.zzz");

            // 使用 HTML 样式让时间戳变细、变灰，突出主体内容
            // 注意：如果是用 appendPlainText 不支持 HTML，需改用 appendHtml
            // 或者简单处理：
            QString finalLog = QString("[%1] %2").arg(timestamp).arg(lineStr);
            m_logDisplay->appendPlainText(finalLog);
        }
    }

    // 自动滚动底部
    m_logDisplay->verticalScrollBar()->setValue(m_logDisplay->verticalScrollBar()->maximum());
}

void SerialDebugWidget::saveLogToFile()
{
    QString logPath = cfg->logFolder();

    if (logPath.isEmpty())
    {
        QString logPath = QCoreApplication::applicationDirPath() + "/logs";
    }

    QDir dir;
    if (!dir.exists(logPath))
    {
        dir.mkpath(logPath);
    }

    // 2. 构造完整文件路径
    QString fullPath = QDir(logPath).filePath("serial.log");

    // 3. 写入文件 (QIODevice::WriteOnly | QIODevice::Text 会覆盖旧文件，
    // 若想追加请使用 QIODevice::Append)
    QFile file(fullPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        out.setCodec("UTF-8"); // 确保编码正确
        out << m_logDisplay->toPlainText();
        file.close();

        // 可选：给用户一个反馈
        m_logDisplay->appendPlainText(tr("--- 日志已保存至: %1 ---").arg(fullPath));
        logger->log(QStringLiteral("SerialDebugWidget"), spdlog::level::info, QStringLiteral("日志保存成功: %1").arg(fullPath));
    }
    else
    {
        m_logDisplay->appendPlainText(tr("--- 错误：无法保存日志文件 ---"));
        logger->log(QStringLiteral("SerialDebugWidget"), spdlog::level::err, QStringLiteral("无法创建文件: %1").arg(fullPath));
    }
}