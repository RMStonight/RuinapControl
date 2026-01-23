#include "components/BottomInfoBar.h"
#include <QGridLayout>
#include <QLabel>
#include <QDebug>
#include "utils/AgvData.h"

// 定义一个简单的结构体用于配置
struct ItemConfig
{
    QString key;
    int colSpan; // 占据几列
};

BottomInfoBar::BottomInfoBar(QWidget *parent) : QWidget(parent)
{
    this->setFixedHeight(120);
    this->setAttribute(Qt::WA_StyledBackground, true);
    this->setStyleSheet(R"(
        BottomInfoBar {
            background-color: #FFFFFF; 
        }
        /* 确保里面的 Label 也是透明背景，显示出 BottomInfoBar 的白色 */
        QLabel {
            background-color: transparent;
        }
    )");
    initLayout();

    // 初始化定时器
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(UPDATE_INTERVAL_MS);
    connect(m_updateTimer, &QTimer::timeout, this, &BottomInfoBar::updateUi);
    m_updateTimer->start();
}

BottomInfoBar::~BottomInfoBar()
{
    if (m_updateTimer->isActive())
    {
        m_updateTimer->stop();
    }
}

void BottomInfoBar::initLayout()
{
    QGridLayout *gridLayout = new QGridLayout(this);
    gridLayout->setContentsMargins(10, 8, 10, 8);
    gridLayout->setVerticalSpacing(0);
    gridLayout->setHorizontalSpacing(8);

    // =======================================================
    // 1. 定义数据列表 (可以在这里指定某些项占据更宽的空间)
    // =======================================================
    QList<ItemConfig> items = {
        // 第一行 (正常 1 格)
        {"起点X", 1},
        {"起点Y", 1},
        {"终点X", 1},
        {"终点Y", 1},
        {"当前点位", 1},
        {"载货状态", 1},
        {"时长T", 1},
        {"里程O", 1},

        // 第二行 (正常 1 格)
        {"速度X", 1},
        {"速度Y", 1},
        {"速度W", 1},
        {"方向D", 1},
        {"任务动作", 1},
        {"急停状态", 1},
        {"任务编号", 2},

        // 第三行 (正常 1 格)
        {"坐标X", 1},
        {"坐标Y", 1},
        {"角度A", 1},
        {"协方差", 1},
        {"可选错误", 1},
        {"任务错误", 1},
        {"任务消息", 2},

        // 第四行 (特殊处理：AGV错误 比较长)
        {"车体错误", 8}};

    int maxColumns = 8; // 每行最大列数
    int currentRow = 0;
    int currentCol = 0;

    // =======================================================
    // 2. 流式布局算法 (替代原来的 i/7 算法)
    // =======================================================
    for (const auto &item : items)
    {
        // 如果当前列位置 + 本控件跨度 > 最大列数，则换行
        if (currentCol + item.colSpan > maxColumns)
        {
            currentRow++;
            currentCol = 0;
        }

        // 添加控件
        addStatusItem(gridLayout, item.key, currentRow, currentCol, item.colSpan);

        // 移动光标
        currentCol += item.colSpan;
    }
}

// 增加 colSpan 参数
void BottomInfoBar::addStatusItem(QGridLayout *layout, const QString &key, int row, int col, int colSpan)
{
    QWidget *itemWidget = new QWidget(this);
    QHBoxLayout *hBox = new QHBoxLayout(itemWidget);
    hBox->setContentsMargins(0, 0, 0, 0);
    hBox->setSpacing(5);

    QLabel *lblKey = new QLabel(key + ":", itemWidget);
    lblKey->setStyleSheet(lblKeyStyle);
    lblKey->setFixedWidth(55);
    lblKey->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QLabel *lblValue = new QLabel("NaN", itemWidget);
    lblValue->setStyleSheet(lblValueStyleGreen);

    // 如果跨度比较大（例如任务消息），可以让 Value Label 自动拉伸，不要被挤到左边
    if (colSpan > 1)
    {
        lblValue->setWordWrap(true); // 允许长文字换行
    }

    hBox->addWidget(lblKey);
    hBox->addWidget(lblValue);

    // 关键点：如果是跨列的长条，我们可能希望内容靠左，右边留白；
    // 或者如果文字很长，stretch 会把文字顶在左边。
    hBox->addStretch();

    m_valueLabels.insert(key, lblValue);

    // =======================================================
    // 关键调用：addWidget(widget, row, col, rowSpan, colSpan)
    // rowSpan 固定为 1，colSpan 使用传入的值
    // =======================================================
    layout->addWidget(itemWidget, row, col, 1, colSpan);
}

void BottomInfoBar::updateValue(const QString &key, const QString &value)
{
    if (m_valueLabels.contains(key))
    {
        QLabel *lbl = m_valueLabels[key];

        // 特殊处理显示内容以及是否标红
        if (key.contains("可选错误") || key.contains("任务错误"))
        {
            if (value == "" || value.isNull())
            {
                lbl->setText("无");
                lbl->setStyleSheet(lblValueStyleGreen);
            }
            else
            {
                lbl->setText("报警");
                lbl->setStyleSheet(lblValueStyleRed);
            }
        }
        else if (key.contains("急停状态"))
        {
            lbl->setText(value);
            if (value == "1")
            {
                lbl->setText("触发");
                lbl->setStyleSheet(lblValueStyleRed);
            }
            else
            {
                lbl->setText("未触发");
                lbl->setStyleSheet(lblValueStyleGreen);
            }
        }
        else if (key.contains("车体错误"))
        {
            if (value == "" || value.isNull())
            {
                lbl->setText("无");
                lbl->setStyleSheet(lblValueStyleGreen);
            }
            else
            {
                lbl->setText(value);
                lbl->setStyleSheet(lblValueStyleRed);
            }
        }
        else
        {
            lbl->setText(value);
        }
    }
}

void BottomInfoBar::updateAllValues(const QMap<QString, QString> &data)
{
    QMapIterator<QString, QString> i(data);
    while (i.hasNext())
    {
        i.next();
        updateValue(i.key(), i.value());
    }
}

void BottomInfoBar::updateUi()
{
    // 获取 AgvData 实例
    AgvData *agvData = AgvData::instance();
    QMap<QString, QString> updateData;
    updateData.insert("起点X", QString::number(agvData->taskStartX().value));
    updateData.insert("起点Y", QString::number(agvData->taskStartY().value));
    updateData.insert("终点X", QString::number(agvData->taskEndX().value));
    updateData.insert("终点Y", QString::number(agvData->taskEndY().value));
    updateData.insert("当前点位", QString::number(agvData->pointId().value));
    updateData.insert("载货状态", handleGoodsState(agvData->goodsState().value));
    updateData.insert("时长T", QString::number(agvData->runTime().value));
    updateData.insert("里程O", QString::number(agvData->runLength().value));
    updateData.insert("速度X", QString::number(agvData->vX().value));
    updateData.insert("速度Y", QString::number(agvData->vY().value));
    updateData.insert("速度W", QString::number(agvData->vAngle().value / 100.0, 'f', 2));
    updateData.insert("方向D", handleMoveDir(agvData->moveDir().value));
    updateData.insert("任务动作", QString::number(agvData->taskAct().value));
    updateData.insert("急停状态", QString::number(agvData->eStopState().value));
    updateData.insert("任务编号", agvData->taskId().value);
    updateData.insert("坐标X", QString::number(agvData->slamX().value));
    updateData.insert("坐标Y", QString::number(agvData->slamY().value));
    updateData.insert("角度A", QString::number(agvData->slamAngle().value / 100.0, 'f', 2));
    updateData.insert("协方差", QString::number(agvData->slamCov().value / 1000.0, 'f', 3));
    updateData.insert("可选错误", agvData->optionalErr().value);
    updateData.insert("任务错误", agvData->taskErr().value);
    updateData.insert("任务消息", agvData->taskDescription().value);
    updateData.insert("车体错误", agvData->agvErr().value);
    updateAllValues(updateData);
}

const QString BottomInfoBar::handleMoveDir(int moveDir)
{
    QString _result = "";
    switch (moveDir)
    {
    case 0:
        _result = "停车";
        break;

    case 1:
        _result = "前进";
        break;

    case 2:
        _result = "后退";
        break;

    case 3:
        _result = "左横移";
        break;

    case 4:
        _result = "右横移";
        break;

    case 5:
        _result = "逆原";
        break;

    case 6:
        _result = "顺原";
        break;

    default:
        break;
    }

    return _result;
}

const QString BottomInfoBar::handleGoodsState(int goodsState)
{
    QString _result = "";
    switch (goodsState)
    {
    case 0:
        _result = "无货";
        break;

    case 1:
        _result = "单左货";
        break;

    case 2:
        _result = "单右货";
        break;

    case 3:
        _result = "左右货";
        break;

    default:
        break;
    }
    return _result;
}