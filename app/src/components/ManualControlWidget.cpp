#include "ManualControlWidget.h"
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDebug>
#include <QGroupBox>

ManualControlWidget::ManualControlWidget(QWidget *parent) : BaseDisplayWidget(parent)
{
    this->setStyleSheet("background-color: #ffffff;");
    initUi();

    // 初始化定时器
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(UPDATE_INTERVAL_MS);
    connect(m_updateTimer, &QTimer::timeout, this, &ManualControlWidget::updateUi);
    m_updateTimer->start();
}

ManualControlWidget::~ManualControlWidget()
{
    if (m_updateTimer->isActive())
    {
        m_updateTimer->stop();
    }
}

void ManualControlWidget::updateUi()
{
    // 检测电量，如果 大于等于充电阈值 且手动充电打开，则关闭它
    if ((agvData->battery().value >= cfg->chargingThreshold()) && chargeCheck->isChecked())
    {
        chargeCheck->setChecked(false);
    }
}

void ManualControlWidget::initUi()
{
    // 创建主容器，设置较宽的边距防止“左侧贴边”
    m_contentContainer = new QWidget(this);
    QVBoxLayout *mainVLayout = new QVBoxLayout(m_contentContainer);
    mainVLayout->setContentsMargins(20, 10, 20, 10); // 增加左边距(30)解决字样显示不全
    mainVLayout->setSpacing(15);

    // 定义统一的标签和复选框样式
    QString labelStyle = "font-size: 14px; color: #333333;";

    // --- 第一行：线速度 (撑满空间) ---
    QHBoxLayout *row1 = new QHBoxLayout();
    QLabel *speedLabel = new QLabel("速度Vx");
    speedLabel->setStyleSheet(labelStyle); // 放大字体
    QSlider *speedSlider = new QSlider(Qt::Horizontal);
    speedSlider->setRange(100, 800);
    QLabel *speedDisplay = new QLabel("0.100 m/s");
    speedDisplay->setStyleSheet(labelStyle); // 放大字体
    speedDisplay->setFixedWidth(75);

    connect(speedSlider, &QSlider::valueChanged, this, [this, speedDisplay](int v)
            {
        agvData->setManualVx(v); // 更新变量
        speedDisplay->setText(QString("%1 m/s").arg(v / 1000.0, 0, 'f', 3)); });

    row1->addWidget(speedLabel);
    row1->addWidget(speedSlider, 1); // 1表示拉伸撑满
    row1->addWidget(speedDisplay);
    mainVLayout->addLayout(row1);
    mainVLayout->addStretch(1);

    // --- 第二行：页面控制与充电控制 (左右对齐) ---
    QHBoxLayout *row2 = new QHBoxLayout();
    QCheckBox *pageCheck = new QCheckBox("页面控制");
    chargeCheck = new QCheckBox("手动充电");

    // 放大复选框文字
    pageCheck->setStyleSheet("QCheckBox { font-size: 14px; } QCheckBox::indicator { width: 20px; height: 20px; }");
    chargeCheck->setStyleSheet("QCheckBox { font-size: 14px; } QCheckBox::indicator { width: 20px; height: 20px; }");

    connect(pageCheck, &QCheckBox::toggled, this, [this](bool checked)
            { agvData->setPageControl(checked); });

    connect(chargeCheck, &QCheckBox::toggled, this, [this](bool checked)
            { agvData->setChargeCmd(checked); });

    row2->addWidget(pageCheck);
    row2->addStretch();
    row2->addWidget(chargeCheck);
    mainVLayout->addLayout(row2);
    mainVLayout->addStretch(1);

    // --- 第三行：移动控制(左) 与 动作控制(右) ---
    QHBoxLayout *row3 = new QHBoxLayout();
    row3->setSpacing(80);

    // 移动控制 (绿色八向)
    QGroupBox *moveBox = new QGroupBox("移动控制");
    QFont moveBoxFont = moveBox->font();
    moveBoxFont.setPointSize(10); // 这里的点大小需根据实际屏幕调整，22px 约等于 16-18pt
    moveBox->setFont(moveBoxFont);

    QGridLayout *moveGrid = new QGridLayout(moveBox);
    const QString green = "#00897B";
    moveGrid->addWidget(createMomentaryButton(1, "↖", green, 8), 0, 0);
    moveGrid->addWidget(createMomentaryButton(1, "↑", green, 2), 0, 1);
    moveGrid->addWidget(createMomentaryButton(1, "↗", green, 7), 0, 2);
    moveGrid->addWidget(createMomentaryButton(1, "←", green, 4), 1, 0);
    moveGrid->addWidget(createMomentaryButton(1, "→", green, 3), 1, 2);
    moveGrid->addWidget(createMomentaryButton(1, "↙", green, 6), 2, 0);
    moveGrid->addWidget(createMomentaryButton(1, "↓", green, 1), 2, 1);
    moveGrid->addWidget(createMomentaryButton(1, "↘", green, 5), 2, 2);

    // 动作控制 (橙色四向)
    QGroupBox *actionBox = new QGroupBox("动作控制");
    QFont actionBoxFont = actionBox->font();
    actionBoxFont.setPointSize(10); // 这里的点大小需根据实际屏幕调整，22px 约等于 16-18pt
    actionBox->setFont(actionBoxFont);

    QGridLayout *actionGrid = new QGridLayout(actionBox);
    const QString orange = "#FFA726";
    actionGrid->addWidget(createMomentaryButton(2, "↑", orange, 1), 0, 1);
    actionGrid->addWidget(createMomentaryButton(2, "←", orange, 3), 1, 0);
    actionGrid->addWidget(createMomentaryButton(2, "→", orange, 4), 1, 2);
    actionGrid->addWidget(createMomentaryButton(2, "↓", orange, 2), 2, 1);

    row3->addStretch(1); // 左侧弹簧
    row3->addWidget(moveBox);
    row3->addWidget(actionBox);
    row3->addStretch(1); // 右侧弹簧，确保两个Box合拢居中
    mainVLayout->addLayout(row3);
    mainVLayout->addStretch(1);

    // --- 第四行：任务按钮 (两两对齐) ---
    QHBoxLayout *row4 = new QHBoxLayout();
    row4->setSpacing(20);

    // 左对齐组
    row4->addWidget(createMomentaryButton(3, "删除装卸", "#F44336", 1));
    row4->addWidget(createMomentaryButton(4, "开始任务", "#4CAF50", 1));

    // 中间弹簧：将两组按钮推向极两端
    row4->addStretch(1);

    // 右对齐组
    row4->addWidget(createMomentaryButton(5, "暂停任务", "#FFC107", 1));
    row4->addWidget(createMomentaryButton(6, "恢复任务", "#4CAF50", 1));

    mainVLayout->addLayout(row4);
}

/**
 * @brief 创建点动（Momentary）按钮
 * @param type 1为控制箭头(方向)，2为控制箭头(动作)，3删除装卸，4开始任务，5暂停任务，6取消任务
 * @param text 按钮文字
 * @param color 背景颜色
 * @param val 按下时赋予的值 (仅对 int 变量有效)
 */
QPushButton *ManualControlWidget::createMomentaryButton(int type, const QString &text, const QString &color, int val)
{
    QPushButton *btn = new QPushButton(text);
    if (type == 1)
    {
        btn->setFixedSize(120, 80);
        btn->setStyleSheet(QString(
                               "QPushButton { background-color: %1; color: white; border-radius: 8px; font-weight: bold; font-size: 36px; }"
                               "QPushButton:pressed { background-color: #cccccc; }")
                               .arg(color));
    }
    else
    {
        btn->setFixedSize(110, 60);
        btn->setStyleSheet(QString(
                               "QPushButton { background-color: %1; color: white; border-radius: 6px; font-weight: bold; font-size: 18px; }"
                               "QPushButton:pressed { background-color: #cccccc; }")
                               .arg(color));
    }

    // 处理按下逻辑
    connect(btn, &QPushButton::pressed, this, [=]()
            {
        if (type == 1) {
            agvData->setManualDir(val);
        }
        else if (type == 2)
        {
            agvData->setManualAct(val);
        } 
        else if (type == 3)
        {
            agvData->setTaskCancel(true);
        } 
        else if (type == 4)
        {
            agvData->setTaskStart(true);
        } 
        else if (type == 5)
        {
            agvData->setTaskPause(true);
        } 
        else if (type == 6)
        {
            agvData->setTaskResume(true);
        } 
        else {

        } });

    // 处理松开逻辑
    connect(btn, &QPushButton::released, this, [=]()
            {
        if (type == 1) {
            // m_manualDir
            agvData->setManualDir(0);
        } 
        else if (type == 2)
        {
            // m_manualAct
            agvData->setManualAct(0);
        } 
        else if (type == 3)
        {
            agvData->setTaskCancel(false);
        } 
        else if (type == 4)
        {
            agvData->setTaskStart(false);
        } 
        else if (type == 5)
        {
            agvData->setTaskPause(false);
        } 
        else if (type == 6)
        {
            agvData->setTaskResume(false);
        } 
        else {

        } });

    return btn;
}

void ManualControlWidget::resizeEvent(QResizeEvent *event)
{
    BaseDisplayWidget::resizeEvent(event);
    if (m_contentContainer)
    {
        int leftWidth = getDrawingWidth();

        // 关键：不要 adjustSize，直接设置容器宽度为 leftWidth
        // 这样容器内部的 layout 才能真正利用这部分空间
        m_contentContainer->setGeometry(0, 10, leftWidth, height() - 20);
    }
}