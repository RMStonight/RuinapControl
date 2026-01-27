#include "OptionalInfoWidget.h"
#include "AgvData.h"
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonDocument>
#include <QVariant>
#include <QDebug>

OptionalInfoWidget::OptionalInfoWidget(QWidget *parent) : QScrollArea(parent)
{
    this->setWidgetResizable(true);
    this->setFrameShape(QFrame::NoFrame);
    this->setStyleSheet("QScrollArea { background-color: transparent; }");

    m_contentWidget = new QWidget(this);
    m_contentWidget->setObjectName("container");
    m_contentWidget->setStyleSheet("#container { background-color: #ffffff;}");

    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(10, 10, 10, 10);
    m_contentLayout->setSpacing(5);

    this->setWidget(m_contentWidget);
    m_contentLayout->addStretch();

    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(UPDATE_INTERVAL_MS);
    connect(m_updateTimer, &QTimer::timeout, this, &OptionalInfoWidget::updateUi);
    m_updateTimer->start();
}

OptionalInfoWidget::~OptionalInfoWidget()
{
    if (m_updateTimer->isActive())
    {
        m_updateTimer->stop();
    }
}

// [修改] 支持颜色参数
void OptionalInfoWidget::setInfoRow(const QString &key, const QString &value, const QString &color)
{
    // 构造样式字符串
    QString styleSheet = QString("color: %1; font-weight: bold; font-size: 12px;").arg(color);

    // 1. 检查该 Key 是否已经存在
    if (m_rowMap.contains(key))
    {
        QLabel *valLabel = m_rowMap[key];

        // 更新文本
        if (valLabel->text() != value)
        {
            valLabel->setText(value);
        }

        // 更新样式 (如果颜色变了，样式表也会变)
        if (valLabel->styleSheet() != styleSheet)
        {
            valLabel->setStyleSheet(styleSheet);
        }
        return;
    }

    // 2. 创建新行
    QWidget *rowWidget = new QWidget(m_contentWidget);
    // 给每一行设置对象名，方便样式表定位（非必须，但在复杂样式下很有用）
    rowWidget->setObjectName("rowWidget");

    // 1. 设置固定高度或最小高度，让行距宽松一点
    rowWidget->setMinimumHeight(28);

    // 2. 斑马纹逻辑：根据当前行数决定背景色
    int count = m_contentLayout->count();                       // 注意：最后有一个 stretch，计算时要注意
    QString bgColor = (count % 2 == 0) ? "#ffffff" : "#f7f8fa"; // 偶数行白，奇数行淡灰

    // 3. 设置样式：背景色 + 底部淡灰色分割线
    rowWidget->setStyleSheet(QString(
                                 "#rowWidget { "
                                 "   background-color: %1; "
                                 "}"
                                 )
                                 .arg(bgColor));
    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *keyLabel = new QLabel(key, rowWidget);
    keyLabel->setStyleSheet("color: #666; font-size: 12px;");

    QLabel *valLabel = new QLabel(value, rowWidget);
    // [应用] 设置包含颜色的样式表
    valLabel->setStyleSheet(styleSheet);
    valLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    rowLayout->addWidget(keyLabel);
    rowLayout->addStretch();
    rowLayout->addWidget(valLabel);

    m_contentLayout->insertWidget(m_contentLayout->count() - 1, rowWidget);

    m_rowMap.insert(key, valLabel);
}

void OptionalInfoWidget::updateUi()
{
    AgvData *agvData = AgvData::instance();
    QJsonObject optionalInfo = agvData->optionalInfo();

    for (auto it = optionalInfo.constBegin(); it != optionalInfo.constEnd(); ++it)
    {
        QString key = it.key();
        QJsonValue jsonVal = it.value();

        QString displayStr;
        QString displayColor = "#000000"; // 默认黑色

        if (jsonVal.isObject())
        {
            QJsonObject obj = jsonVal.toObject();
            // 检查是否符合 { "value": ..., "color": ... } 结构
            if (obj.contains("value"))
            {
                displayStr = obj["value"].toVariant().toString();

                // [新增] 提取颜色
                if (obj.contains("color"))
                {
                    displayColor = obj["color"].toString("#000000");
                }
            }
            else
            {
                // 普通对象全量显示
                QJsonDocument doc(obj);
                displayStr = QString(doc.toJson(QJsonDocument::Compact));
            }
        }
        else
        {
            // 基本类型
            displayStr = jsonVal.toVariant().toString();
        }

        // [修改] 传入解析出的颜色
        setInfoRow(key, displayStr, displayColor);
    }
}