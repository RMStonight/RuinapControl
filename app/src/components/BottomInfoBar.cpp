#include "components/BottomInfoBar.h"
// =========================================================
// [关键修复] 必须包含该头文件，否则 .cpp 中无法调用 addWidget
// =========================================================
#include <QGridLayout>
#include <QLabel>
#include <QDebug>

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
        {"起点 X", 1},
        {"起点 Y", 1},
        {"终点 X", 1},
        {"终点 Y", 1},
        {"当前点位", 1},
        {"载货状态", 1},
        {"时长 T", 1},
        {"里程 O", 1},

        // 第二行 (正常 1 格)
        {"速度 X", 1},
        {"速度 Y", 1},
        {"速度 W", 1},
        {"方向 D", 1},
        {"任务动作", 1},
        {"急停状态", 1},
        {"任务编号", 2},

        // 第三行 (正常 1 格)
        {"坐标 X", 1},
        {"坐标 Y", 1},
        {"角度 A", 1},
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
    lblKey->setStyleSheet("color: #666666; font-size: 12px;");
    lblKey->setFixedWidth(55);
    lblKey->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QLabel *lblValue = new QLabel("0", itemWidget);
    lblValue->setStyleSheet("color: #009900; font-weight: bold; font-size: 13px;");

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
        lbl->setText(value);

        // 简单的告警逻辑：如果是错误字段且值不为0，标红
        if ((key.contains("错误") || key.contains("状态")) && value != "0")
        {
            lbl->setStyleSheet("color: red; font-weight: bold; font-size: 13px;");
        }
        else
        {
            lbl->setStyleSheet("color: #009900; font-weight: bold; font-size: 13px;");
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