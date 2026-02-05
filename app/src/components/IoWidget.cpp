#include "IoWidget.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include "utils/AgvData.h"

IoWidget::IoWidget(QWidget *parent) : BaseDisplayWidget(parent)
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
    // 我们创建一个透明的容器来装所有的灯，方便整体位移
    m_scrollContainer = new QWidget(this);

    QVBoxLayout *containerLayout = new QVBoxLayout(m_scrollContainer);
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setSpacing(15);

    // --- X 信号 & Y 信号 生成逻辑保持不变 ---
    for (int i = 0; i < 24; ++i)
    {
        QLabel *lamp = createLamp(QString("X%1").arg(i + 1));
        m_xLamps.append(lamp);
        gridLayout->addWidget(lamp, i / 8, i % 8);
    }

    QSpacerItem *verticalSpacer = new QSpacerItem(20, 60, QSizePolicy::Minimum, QSizePolicy::Fixed);
    gridLayout->addItem(verticalSpacer, 3, 0);

    for (int i = 0; i < 24; ++i)
    {
        QLabel *lamp = createLamp(QString("Y%1").arg(i + 1));
        m_yLamps.append(lamp);
        gridLayout->addWidget(lamp, 4 + (i / 8), i % 8);
    }

    containerLayout->addLayout(gridLayout);
    m_scrollContainer->adjustSize(); // 根据灯的数量自适应大小
}

// 重写绘图或布局逻辑以确保 IO 面板在 3/4 区域内居中
void IoWidget::resizeEvent(QResizeEvent *event)
{
    BaseDisplayWidget::resizeEvent(event); // 先执行基类侧边栏布局

    if (m_scrollContainer) {
        int leftWidth = getDrawingWidth(); // 获取左侧 3/4 宽度
        int centerX = leftWidth / 2;
        int centerY = height() / 2;

        // 将 IO 面板移动到左侧区域的中心
        m_scrollContainer->move(centerX - m_scrollContainer->width() / 2, 
                                centerY - m_scrollContainer->height() / 2);
    }
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