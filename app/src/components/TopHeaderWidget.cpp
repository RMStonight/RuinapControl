// src/components/TopHeaderWidget.cpp
#include "components/TopHeaderWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPixmap>
#include <QFile>
#include <QStyle>
#include <QVariant>
#include <QPainter>
#include "utils/ConfigManager.h"

TopHeaderWidget::TopHeaderWidget(QWidget *parent) : QWidget(parent)
{
    // 设置自身属性
    this->setFixedHeight(100);
    this->setObjectName("headerContainer");

    // 加载 QSS 文件
    QFile file(":/styles/top_header.qss");
    if (file.open(QFile::ReadOnly))
    {
        QString styleSheet = QLatin1String(file.readAll());
        this->setStyleSheet(styleSheet);
        file.close();
    }

    initLayout();

    // 初始化时，直接读取配置显示
    updateInfoFromConfig();
    // 监听配置修改信号
    // 当 SystemSettingsWidget 保存后，ConfigManager 发出信号，这里自动刷新
    connect(ConfigManager::instance(), &ConfigManager::configChanged,
            this, &TopHeaderWidget::updateInfoFromConfig);
}

void TopHeaderWidget::initLayout()
{
    QHBoxLayout *topLayout = new QHBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(0);

    // ==========================================
    // 【左侧容器】
    // ==========================================
    QWidget *leftContainer = new QWidget(this);
    leftContainer->setFixedWidth(280);

    QHBoxLayout *leftLayout = new QHBoxLayout(leftContainer);
    leftLayout->setContentsMargins(20, 0, 0, 0);
    leftLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // 改为从内存单例读取
    ConfigManager *cfg = ConfigManager::instance();
    m_logoLabel = new QLabel(this);
    QString resourceFolder = cfg->resourceFolder();
    // 如果相对路径不是以 / 结尾则需要添加
    if (!resourceFolder.endsWith("/"))
    {
        resourceFolder += "/";
    }

    QPixmap logo(resourceFolder + TOP_LEFT_LOGO);
    if (!logo.isNull())
    {
        m_logoLabel->setPixmap(logo.scaled(200, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    else
    {
        m_logoLabel->setText("Logo");
    }
    leftLayout->addWidget(m_logoLabel);

    // ==========================================
    // 【中间容器】
    // ==========================================
    QWidget *centerContainer = new QWidget(this);
    centerContainer->setStyleSheet("background-color: transparent;");

    // 中间容器的主布局
    QHBoxLayout *centerMainLayout = new QHBoxLayout(centerContainer);
    centerMainLayout->setContentsMargins(0, 0, 0, 0);
    centerMainLayout->setSpacing(10); // 设定统一间距
    centerMainLayout->setAlignment(Qt::AlignCenter);

    // 定义统一的大小，用于平衡左右
    const int ICON_SIZE = 48; // 稍微调大一点，留出余量

    // 左侧：透明占位符 (Invisible Dummy Widget)
    // 它的唯一作用就是占位，平衡右侧的灯，让中间的文字绝对居中
    QWidget *dummyLeftWidget = new QWidget(this);
    dummyLeftWidget->setFixedSize(ICON_SIZE, ICON_SIZE);
    // 不需要设置任何内容，默认透明

    // 中间：文字包裹层 (Text Wrapper)
    QWidget *textWrapper = new QWidget(this);
    QVBoxLayout *textLayout = new QVBoxLayout(textWrapper);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(5);
    textLayout->setAlignment(Qt::AlignCenter);

    m_agvIdLabel = new QLabel(this);
    m_ipLabel = new QLabel(this);
    setAgvInfo("1", "192.168.160.231"); // 初始值

    QFont fontId;
    fontId.setBold(true);
    fontId.setPointSize(25);
    m_agvIdLabel->setFont(fontId);
    m_agvIdLabel->setAlignment(Qt::AlignCenter);

    QFont fontIp;
    fontIp.setBold(true);
    fontIp.setPointSize(12);
    m_ipLabel->setFont(fontIp);
    m_ipLabel->setAlignment(Qt::AlignCenter);

    textLayout->addWidget(m_agvIdLabel);
    textLayout->addWidget(m_ipLabel);

    // 右侧：Light 图标
    m_lightLabel = new QLabel(this);
    m_lightLabel->setFixedSize(ICON_SIZE, ICON_SIZE); // 宽度必须与 dummyLeftWidget 一致
    m_lightLabel->setScaledContents(true);
    setLightColor("#ffffff"); // 初始灰色

    // 组装布局
    // 结构：[Stretch] [Dummy] [Text] [Light] [Stretch]
    // 这样 [Dummy+Text+Light] 作为一个整体居中，
    // 而因为 Dummy 和 Light 等宽，Text 就在该整体的正中间。
    centerMainLayout->addStretch();
    centerMainLayout->addWidget(dummyLeftWidget);
    centerMainLayout->addWidget(textWrapper);
    centerMainLayout->addWidget(m_lightLabel);
    centerMainLayout->addStretch();

    // ==========================================
    // 【右侧容器】
    // ==========================================
    QWidget *rightContainer = new QWidget(this);
    rightContainer->setFixedWidth(280);

    QVBoxLayout *rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 10, 10, 10);
    rightLayout->setSpacing(5);
    rightLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // 设置右侧字体
    QFont labelFont;
    labelFont.setPointSize(10);
    labelFont.setBold(true);
    uint8_t labelNameWidth = 80;
    uint8_t labelValueWidth = 50;

    // --- 备用字段 ---
    QWidget *reserveWidget = new QWidget(this);
    QHBoxLayout *reserveLayout = new QHBoxLayout(reserveWidget);
    reserveLayout->setContentsMargins(0, 0, 0, 0);
    reserveLayout->setSpacing(0);               // 紧凑排列
    reserveLayout->setAlignment(Qt::AlignRight); // 整体右对齐

    m_reserveLabel = new QLabel("备用字段", this);
    m_reserveLabel->setFont(labelFont);
    m_reserveLabel->setFixedWidth(labelNameWidth + labelValueWidth); // 【关键】设定名称固定宽度

    reserveLayout->addWidget(m_reserveLabel);

    // --- 运行模式行 (水平布局) ---
    QWidget *runModeWidget = new QWidget(this);
    QHBoxLayout *runModeLayout = new QHBoxLayout(runModeWidget);
    runModeLayout->setContentsMargins(0, 0, 0, 0);
    runModeLayout->setSpacing(0);                // 紧凑排列
    runModeLayout->setAlignment(Qt::AlignRight); // 整体右对齐

    m_runModeNameLabel = new QLabel("运行模式：", this);
    m_runModeNameLabel->setFont(labelFont);
    m_runModeNameLabel->setFixedWidth(labelNameWidth); // 【关键】设定名称固定宽度
    m_runModeNameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_runModeValueLabel = new QLabel("自动", this);
    m_runModeValueLabel->setFont(labelFont);
    m_runModeValueLabel->setFixedWidth(labelValueWidth);                 // 【关键】设定值固定宽度，防止跳动
    m_runModeValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter); // 值左对齐

    runModeLayout->addWidget(m_runModeNameLabel);
    runModeLayout->addWidget(m_runModeValueLabel);

    // --- 车辆状态行 (水平布局) ---
    QWidget *agvStatusWidget = new QWidget(this);
    QHBoxLayout *agvStatusLayout = new QHBoxLayout(agvStatusWidget);
    agvStatusLayout->setContentsMargins(0, 0, 0, 0);
    agvStatusLayout->setSpacing(0);
    agvStatusLayout->setAlignment(Qt::AlignRight);

    m_agvStatusNameLabel = new QLabel("当前状态：", this);
    m_agvStatusNameLabel->setFont(labelFont);
    m_agvStatusNameLabel->setFixedWidth(labelNameWidth); // 【关键】与上面对齐
    m_agvStatusNameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_agvStatusValueLabel = new QLabel("行走中", this);
    m_agvStatusValueLabel->setFont(labelFont);
    m_agvStatusValueLabel->setFixedWidth(labelValueWidth); // 【关键】与上面对齐
    m_agvStatusValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    agvStatusLayout->addWidget(m_agvStatusNameLabel);
    agvStatusLayout->addWidget(m_agvStatusValueLabel);

    // 电池条
    // 为了让电池条也右对齐并且看起来整齐，可以把它放在一个 HLayout 中或者直接添加
    // 这里为了保持与上方文字对齐的视觉效果，可以保留原有逻辑，或者也包裹一层
    m_batteryBar = new QProgressBar(this);
    m_batteryBar->setRange(0, 100);
    m_batteryBar->setFixedWidth(labelNameWidth + labelValueWidth); // 稍微加宽一点以匹配上面的总宽度 (80+80=160 approx) 或者保持原样
    m_batteryBar->setFixedHeight(16);
    setBatteryLevel(85);

    rightLayout->addWidget(reserveWidget);
    rightLayout->addWidget(runModeWidget);
    rightLayout->addWidget(agvStatusWidget);
    rightLayout->addWidget(m_batteryBar);

    // ==========================================
    // 加入主布局
    // ==========================================
    topLayout->addWidget(leftContainer);
    topLayout->addWidget(centerContainer);
    topLayout->addWidget(rightContainer);
}

// 私有槽函数
void TopHeaderWidget::updateInfoFromConfig()
{
    ConfigManager *cfg = ConfigManager::instance();
    // 使用读取到的配置更新 UI
    setAgvInfo(cfg->agvId(), cfg->agvIp());
}

// 实现公开接口
void TopHeaderWidget::setAgvInfo(const QString &id, const QString &ip)
{
    m_agvIdLabel->setText(QString("AGV编号：<span style='color: #016f56;'>%1</span>").arg(id));
    m_ipLabel->setText(QString("IP：<span style='color: #016f56;'>%1</span>").arg(ip));
}

void TopHeaderWidget::setBatteryLevel(int level)
{
    m_batteryBar->setValue(level);

    // 判断当前状态
    QString state;
    if (level >= 75)
    {
        state = "normal"; // 绿色
    }
    else if (level >= 20)
    {
        state = "warning"; // 黄色
    }
    else
    {
        state = "critical"; // 红色
    }

    // 仅当状态发生改变时才更新属性，避免不必要的重绘
    if (m_batteryBar->property("batteryState").toString() != state)
    {
        m_batteryBar->setProperty("batteryState", state);

        // 强制刷新样式（Qt QSS 的机制：属性改变后需要 unpolish/polish 才能触发选择器更新）
        m_batteryBar->style()->unpolish(m_batteryBar);
        m_batteryBar->style()->polish(m_batteryBar);
    }
}

void TopHeaderWidget::setRunMode(const QString &mode)
{
    m_runModeValueLabel->setText(mode);
}

void TopHeaderWidget::setAgvStatus(const QString &status)
{
    m_agvStatusValueLabel->setText(status);
}

void TopHeaderWidget::setLightColor(const QString &colorStr)
{
    // 假设 light.svg 已经在资源文件中，路径为 :/images/light.svg
    // 如果不在，请修改为你实际的资源路径
    static const QString iconPath = ":/icons/light.svg";

    QPixmap src(iconPath);
    if (src.isNull())
    {
        return; // 防止崩溃
    }

    QColor color(colorStr);
    if (!color.isValid())
    {
        color = QColor("#aaaaaa"); // 默认色
    }

    // 调用染色辅助函数
    QPixmap coloredPix = colorizePixmap(src, color);
    m_lightLabel->setPixmap(coloredPix);
}

QPixmap TopHeaderWidget::colorizePixmap(const QPixmap &src, const QColor &color)
{
    // 创建一个和原图一样大小的空 Pixmap
    QPixmap result(src.size());
    result.fill(Qt::transparent); // 背景透明

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 先把原图画上去
    painter.drawPixmap(0, 0, src);

    // 设置混合模式为 SourceIn
    // 含义：在目标（原图）不透明的地方，绘制源（我们的颜色）。透明的地方保持透明。
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);

    // 用颜色填充整个矩形
    painter.fillRect(result.rect(), color);

    painter.end();
    return result;
}