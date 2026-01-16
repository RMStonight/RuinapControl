#include "MainWindow.h"
#include "utils/ConfigManager.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 构造函数里一般只做顶层调用，保持整洁
    initUI();
    setupConnections();
    applyWindowState();
}

MainWindow::~MainWindow()
{
    // Qt 的对象树机制会自动释放作为子对象的控件，
    // 所以这里通常不需要手动 delete m_logoLabel 等。
}

void MainWindow::initUI()
{
    this->setWindowTitle("Ruinap Control System");
    this->setFixedSize(1280, 800);

    QWidget *centralWidget = new QWidget(this);
    this->setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ==========================================
    // 区域一：顶部 Header (一行代码搞定！)
    // ==========================================
    m_topHeader = new TopHeaderWidget(this);

    // ==========================================
    // 区域二：底部内容
    // ==========================================
    m_mainContent = new MainContentWidget(this);

    // 加入主布局
    mainLayout->addWidget(m_topHeader);
    mainLayout->addWidget(m_mainContent);
}

void MainWindow::setupConnections()
{
    // 示例：点击按钮修改 Header 里的数据，验证模块化是否成功
    connect(m_mainContent, &MainContentWidget::testBtnClicked, [this]()
            {
        m_topHeader->setBatteryLevel(rand() % 100); // 随机设置电量
        m_topHeader->setTaskStatus("充电中..."); });

    // 【新增】监听配置修改信号，实时切换全屏/窗口模式
    connect(ConfigManager::instance(), &ConfigManager::configChanged,
            this, &MainWindow::applyWindowState);
}

// 
void MainWindow::applyWindowState()
{
    bool isFull = ConfigManager::instance()->fullScreen();

    if (isFull) {
        // 真正的全屏：无边框、无标题栏、覆盖任务栏
        this->showFullScreen();
    } else {
        // 恢复正常窗口模式
        this->showNormal();
        
        // 如果你希望非全屏时也是最大化的（但有标题栏），可以用：
        // this->showMaximized(); 
    }
}