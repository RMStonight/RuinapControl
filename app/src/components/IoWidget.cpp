#include "IoWidget.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QDebug>
#include "utils/AgvData.h"

IoWidget::IoWidget(QWidget *parent) : QWidget(parent)
{
    initUi();

    // 初始化定时器
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(UPDATE_INTERVAL_MS);
    connect(m_updateTimer, &QTimer::timeout, this, &IoWidget::updateIoStatus);
    m_updateTimer->start();
}

IoWidget::~IoWidget()
{
    if (m_updateTimer->isActive())
    {
        m_updateTimer->stop();
    }
}

void IoWidget::initUi()
{
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 标题 (可选，根据图2风格)
    mainLayout->addSpacing(20);

    // 网格布局用于放置圆圈
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setSpacing(15); // 灯之间的间距

    // --- X 信号 (输入) ---
    // 24个信号，每行8个 -> 占用行 0, 1, 2
    for (int i = 0; i < 24; ++i)
    {
        QString text = QString("X%1").arg(i + 1);
        QLabel *lamp = createLamp(text);
        m_xLamps.append(lamp);

        int row = i / 8; // 0-7是第0行, 8-15是第1行...
        int col = i % 8; // 0-7 列
        gridLayout->addWidget(lamp, row, col);
    }

    // --- 添加一个分割线或间距 ---
    // 在 X 和 Y 之间增加一行空白，防止视觉混淆
    // QSpacerItem(宽, 高)
    QSpacerItem *verticalSpacer = new QSpacerItem(20, 60, QSizePolicy::Minimum, QSizePolicy::Fixed);
    // 将间隔项添加到第3行 (X占用了0-2)
    gridLayout->addItem(verticalSpacer, 3, 0);

    // --- Y 信号 (输出) ---
    // 同样每行8个 -> 从第 4 行开始 (0,1,2是X, 3是空行)
    int yStartRow = 4;
    for (int i = 0; i < 24; ++i)
    {
        QString text = QString("Y%1").arg(i + 1);
        QLabel *lamp = createLamp(text);
        m_yLamps.append(lamp);

        int row = yStartRow + (i / 8);
        int col = i % 8;
        gridLayout->addWidget(lamp, row, col);
    }

    // 将网格居中放置
    // 左右增加弹簧，让网格在水平方向居中
    QHBoxLayout *hBox = new QHBoxLayout();
    hBox->addStretch();
    hBox->addLayout(gridLayout);
    hBox->addStretch();

    mainLayout->addLayout(hBox);
    mainLayout->addStretch(); // 底部弹簧，顶起内容
}

QLabel *IoWidget::createLamp(const QString &text)
{
    QLabel *lbl = new QLabel(text, this);
    lbl->setFixedSize(50, 50); // 设置圆形的大小
    lbl->setAlignment(Qt::AlignCenter);

    // 默认样式：红色（未触发），圆形边框
    // border-radius 必须是宽高的一半才能由方变圆
    lbl->setStyleSheet(
        "QLabel {"
        "background-color: #dc3545;" // 红色
        "color: white;"
        "border-radius: 25px;" // 50px的一半
        "font-weight: bold;"
        "font-family: Arial;"
        "}");
    return lbl;
}

void IoWidget::setLampState(QLabel *lamp, bool isOn)
{
    // 只有状态改变时才刷新样式，节省性能
    bool currentStatus = lamp->property("isOn").toBool();
    if (currentStatus != isOn)
    {
        lamp->setProperty("isOn", isOn);

        if (isOn)
        {
            // 触发状态：绿色
            lamp->setStyleSheet(
                "QLabel {"
                "background-color: #28a745;" // 绿色
                "color: white;"
                "border-radius: 25px;"
                "font-weight: bold;"
                "}");
        }
        else
        {
            // 未触发状态：红色
            lamp->setStyleSheet(
                "QLabel {"
                "background-color: #dc3545;" // 红色
                "color: white;"
                "border-radius: 25px;"
                "font-weight: bold;"
                "}");
        }
    }
}

void IoWidget::updateIoStatus()
{
    // 定义一个Lambda函数来处理每组8位的逻辑
    // val: 全局变量值, offset: 在Lamps数组中的起始索引, lampList: 对应的UI列表
    auto processBits = [&](int val, int offset, QVector<QLabel *> &lampList)
    {
        for (int bit = 0; bit < 8; ++bit)
        {
            // 提取第 bit 位 (0 或 1)
            bool isActive = (val >> bit) & 1;

            int index = offset + bit;
            if (index < lampList.size())
            {
                setLampState(lampList[index], isActive);
            }
        }
    };

    // 获取 AgvData 实例
    AgvData *agvData = AgvData::instance();
    int agvXin1 = agvData->agvXin1().value;
    int agvXin2 = agvData->agvXin2().value;
    int agvXin3 = agvData->agvXin3().value;
    int agvYout1 = agvData->agvYout1().value;
    int agvYout2 = agvData->agvYout2().value;
    int agvYout3 = agvData->agvYout3().value;

    // --- 更新 X 信号 (输入) ---
    // agvXin1 对应 X1-X8 (索引 0-7)
    processBits(agvXin1, 0, m_xLamps);
    // agvXin2 对应 X9-X16 (索引 8-15)
    processBits(agvXin2, 8, m_xLamps);
    // agvXin3 对应 X17-X24 (索引 16-23)
    processBits(agvXin3, 16, m_xLamps);

    // --- 更新 Y 信号 (输出) ---
    // agvYout1 对应 Y1-Y8 (索引 0-7)
    processBits(agvYout1, 0, m_yLamps);
    // agvYout2 对应 Y9-Y16 (索引 8-15)
    processBits(agvYout2, 8, m_yLamps);
    // agvYout3 对应 Y17-Y24 (索引 16-23)
    processBits(agvYout3, 16, m_yLamps);
}