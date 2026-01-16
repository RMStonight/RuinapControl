#include "components/SystemSettingsWidget.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QSettings>
#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include "utils/ConfigManager.h"
#include <QApplication>
#include <QMessageBox>

SystemSettingsWidget::SystemSettingsWidget(QWidget *parent) : QWidget(parent)
{
    initUI();
    loadSettings(); // 构造时自动加载已有配置
}

void SystemSettingsWidget::initUI()
{
    // 从 QSS 文件加载
    QFile file(":/styles/system_settings.qss");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream stream(&file);
        this->setStyleSheet(stream.readAll());
        file.close();
    }
    else
    {
        qWarning() << "Error: Failed to load stylesheet from :/styles/main_content.qss";
    }

    // 设置主窗口布局 (最外层)
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0); // 去除外层边距，让滚动条贴边
    mainLayout->setSpacing(0);

    // 创建滚动区域
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);       // 关键：让内部 Widget 自动撑满宽度
    scrollArea->setFrameShape(QFrame::NoFrame); // 去掉滚动区的边框，更美观

    // 创建滚动区域的内部容器 (Content Widget)
    QWidget *scrollContent = new QWidget();
    scrollContent->setStyleSheet("background-color: transparent;");

    // 内部容器的垂直布局
    QVBoxLayout *contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setContentsMargins(40, 10, 40, 10); // 内容与四周的留白
    contentLayout->setSpacing(20);                     // 各个板块之间的间距

    // ==========================================
    // 第一部分：AGV 参数设置 [AGV]
    // ==========================================
    contentLayout->addWidget(createSectionLabel("AGV 本体参数")); // 添加标题

    QFormLayout *agvLayout = new QFormLayout();
    agvLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    agvLayout->setVerticalSpacing(10); // 行间距

    // 初始化控件
    m_agvIdEdit = new QLineEdit(this);
    m_agvIdEdit->setPlaceholderText("1");
    m_agvIdEdit->setFixedWidth(100);

    m_agvIpEdit = new QLineEdit(this);
    m_agvIpEdit->setPlaceholderText("192.168.1.100");
    m_agvIpEdit->setFixedWidth(200);

    m_maxSpeedBox = new QSpinBox(this);
    m_maxSpeedBox->setRange(0, 500);   // cm/s
    m_maxSpeedBox->setSuffix(" cm/s"); // 显示单位
    m_maxSpeedBox->setFixedWidth(120);

    agvLayout->addRow("AGV 编号:", m_agvIdEdit);
    agvLayout->addRow("AGV IP:", m_agvIpEdit);
    agvLayout->addRow("最大速度:", m_maxSpeedBox);

    contentLayout->addLayout(agvLayout); // 加入总布局

    // ==========================================
    // 第二部分：文件路径 [FileFolder]
    // ==========================================
    contentLayout->addWidget(createSectionLabel("文件路径设置"));

    QFormLayout *folderLayout = new QFormLayout();
    folderLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    folderLayout->setVerticalSpacing(10);

    m_resourceFolderEdit = new QLineEdit(this);
    m_resourceFolderEdit->setPlaceholderText("/home/ruinap/config_qt/resource");
    m_resourceFolderEdit->setFixedWidth(200);
    folderLayout->addRow("Resource Folder:", m_resourceFolderEdit);

    m_mapPngFolderEdit = new QLineEdit(this);
    m_mapPngFolderEdit->setPlaceholderText("/home/ruinap/maps");
    m_mapPngFolderEdit->setFixedWidth(200);
    folderLayout->addRow("MapPng Folder:", m_mapPngFolderEdit);

    contentLayout->addLayout(folderLayout);

    // ==========================================
    // 第三部分：网络配置 [Network]
    // ==========================================
    contentLayout->addWidget(createSectionLabel("网络通信设置"));

    QFormLayout *netLayout = new QFormLayout();
    netLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    netLayout->setVerticalSpacing(10);

    m_commIpEdit = new QLineEdit(this);
    m_commIpEdit->setPlaceholderText("192.168.1.1");
    m_commIpEdit->setFixedWidth(200);

    m_commPortBox = new QSpinBox(this);
    m_commPortBox->setRange(1, 65535);
    m_commPortBox->setFixedWidth(100);

    netLayout->addRow("数据通讯IP:", m_commIpEdit);
    netLayout->addRow("数据通讯端口:", m_commPortBox);

    m_rosBridgeIpEdit = new QLineEdit(this);
    m_rosBridgeIpEdit->setPlaceholderText("192.168.1.1");
    m_rosBridgeIpEdit->setFixedWidth(200);

    m_rosBridgePortBox = new QSpinBox(this);
    m_rosBridgePortBox->setRange(1, 65535);
    m_rosBridgePortBox->setFixedWidth(100);

    netLayout->addRow("RosBridge IP:", m_rosBridgeIpEdit);
    netLayout->addRow("RosBridge 端口:", m_rosBridgePortBox);

    m_serverIpEdit = new QLineEdit(this);
    m_serverIpEdit->setPlaceholderText("192.168.1.1");
    m_serverIpEdit->setFixedWidth(200);

    m_serverPortBox = new QSpinBox(this);
    m_serverPortBox->setRange(1, 65535);
    m_serverPortBox->setFixedWidth(100);

    netLayout->addRow("服务器 IP:", m_serverIpEdit);
    netLayout->addRow("服务器端口:", m_serverPortBox);

    contentLayout->addLayout(netLayout);

    // ==========================================
    // 第四部分：系统选项 [System]
    // ==========================================
    contentLayout->addWidget(createSectionLabel("系统运行选项"));

    QVBoxLayout *sysLayout = new QVBoxLayout(); // Checkbox 用垂直布局更好看
    sysLayout->setSpacing(10);
    // 给 Checkbox 左侧留点空白，对齐上面的输入框
    sysLayout->setContentsMargins(20, 0, 0, 0);

    m_autoConnectCheck = new QCheckBox("启动时自动连接服务器", this);
    m_debugModeCheck = new QCheckBox("开启调试日志 (Debug Log)", this);
    m_fullScreenCheck = new QCheckBox("开启全屏模式 (隐藏标题栏)", this);
    // 稍微加大一点 Checkbox 的字体
    QString checkStyle = "QCheckBox { font-size: 14px; color: #555; }";
    m_autoConnectCheck->setStyleSheet(checkStyle);
    m_debugModeCheck->setStyleSheet(checkStyle);
    m_fullScreenCheck->setStyleSheet(checkStyle);

    // 添加到表单
    sysLayout->addWidget(m_autoConnectCheck);
    sysLayout->addWidget(m_debugModeCheck);
    sysLayout->addWidget(m_fullScreenCheck);

    contentLayout->addLayout(sysLayout);

    // 底部弹簧：如果内容很少，把它们顶上去，不要分散在整个页面
    contentLayout->addStretch();

    // 将 Content 设置给 ScrollArea
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea); // 将滚动区加入主布局

    // ==========================================
    // 底部固定区域
    // ==========================================
    QWidget *bottomBar = new QWidget(this);
    bottomBar->setObjectName("bottomBar"); // 起个名字
    bottomBar->setFixedHeight(60);         // 固定高度

    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(15, 0, 40, 0);

    // 版本号 Label
    // 使用 qApp->applicationVersion() 动态获取，不要写死在这里
    QLabel *versionLabel = new QLabel(QString("Ver: %1").arg(qApp->applicationVersion()), bottomBar);
    // 设置样式：灰色、小字体、左边距隔开一点
    versionLabel->setStyleSheet("color: #C0C4CC; font-size: 11px; margin-left: 15px; font-family: Arial;");
    versionLabel->setObjectName("versionLabel");
    bottomLayout->addWidget(versionLabel);

    // 添加一点间距，把版本号和提示语分开
    bottomLayout->addSpacing(15);

    // 添加一些说明文字在左边（可选）
    QLabel *tipLabel = new QLabel("修改配置后请点击保存，部分设置重启生效。", bottomBar);
    tipLabel->setObjectName("tipLabel");
    bottomLayout->addWidget(tipLabel);

    bottomLayout->addStretch(); // 弹簧

    // 退出系统按钮
    m_exitBtn = new QPushButton("退出系统", bottomBar);
    m_exitBtn->setFixedSize(120, 40);
    m_exitBtn->setObjectName("btnExit"); // 设置 ID 供 QSS 使用
    bottomLayout->addWidget(m_exitBtn);

    bottomLayout->addSpacing(20);

    // 保存按钮 (保持在最右侧)
    m_saveBtn = new QPushButton("保存所有配置", bottomBar);
    m_saveBtn->setFixedSize(120, 40);
    // 设置对象名 (Object Name)
    // 这样在 QSS 里就可以用 #btnSave 来精确定位它
    m_saveBtn->setObjectName("btnSave");
    bottomLayout->addWidget(m_saveBtn);

    mainLayout->addWidget(bottomBar);

    // 连接保存信号
    connect(m_saveBtn, &QPushButton::clicked, this, &SystemSettingsWidget::saveSettings);

    // 退出逻辑：必须弹窗确认
    connect(m_exitBtn, &QPushButton::clicked, this, [this]()
            {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "系统提示", 
                                      "确定要退出 Ruinap 控制系统吗？\nAGV 连接将会断开。",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            qApp->quit(); // 退出应用程序
        } });
}

// 辅助函数实现
QLabel *SystemSettingsWidget::createSectionLabel(const QString &text)
{
    QLabel *label = new QLabel(text);

    // 设置一个自定义属性，比如叫 "role"，值为 "sectionTitle"
    label->setProperty("role", "sectionTitle");

    return label;
}

// 载入配置
void SystemSettingsWidget::loadSettings()
{
    // 改为从内存单例读取
    ConfigManager *cfg = ConfigManager::instance();

    // 车体参数
    m_agvIdEdit->setText(cfg->agvId());
    m_agvIpEdit->setText(cfg->agvIp());
    m_maxSpeedBox->setValue(cfg->maxSpeed());
    // 文件夹路径
    m_resourceFolderEdit->setText(cfg->resourceFolder());
    m_mapPngFolderEdit->setText(cfg->mapPngFolder());
    // 网络通讯
    m_commIpEdit->setText(cfg->commIp());
    m_commPortBox->setValue(cfg->commPort());
    m_rosBridgeIpEdit->setText(cfg->rosBridgeIp());
    m_rosBridgePortBox->setValue(cfg->rosBridgePort());
    m_serverIpEdit->setText(cfg->serverIp());
    m_serverPortBox->setValue(cfg->serverPort());
    // 系统选项
    m_autoConnectCheck->setChecked(cfg->autoConnect());
    m_debugModeCheck->setChecked(cfg->debugMode());
    m_fullScreenCheck->setChecked(cfg->fullScreen());
}

// 保存配置
void SystemSettingsWidget::saveSettings()
{
    ConfigManager *cfg = ConfigManager::instance();

    // 1. 将 UI 的值写入单例内存
    // 车体参数
    cfg->setAgvId(m_agvIdEdit->text());
    cfg->setAgvIp(m_agvIpEdit->text());
    cfg->setMaxSpeed(m_maxSpeedBox->value());
    // 文件夹路径
    cfg->setResourceFolder(m_resourceFolderEdit->text());
    cfg->setMapPngFolder(m_mapPngFolderEdit->text());
    // 网络通讯
    cfg->setCommIp(m_commIpEdit->text());
    cfg->setCommPort(m_commPortBox->value());
    cfg->setRosBridgeIp(m_rosBridgeIpEdit->text());
    cfg->setRosBridgePort(m_rosBridgePortBox->value());
    cfg->setServerIp(m_serverIpEdit->text());
    cfg->setServerPort(m_serverPortBox->value());
    // 系统选项
    cfg->setAutoConnect(m_autoConnectCheck->isChecked());
    cfg->setDebugMode(m_debugModeCheck->isChecked());
    cfg->setFullScreen(m_fullScreenCheck->isChecked());

    // 2. 调用单例的保存（写入磁盘 + 发送信号）
    cfg->save();

    QMessageBox::information(this, "提示", "系统配置已保存！\n顶部状态栏应已自动更新。");
}