#include "VehicleInfoWidget.h"
#include <QPainter>
#include "utils/AgvData.h"
#include "utils/ConfigManager.h"

VehicleInfoWidget::VehicleInfoWidget(QWidget *parent) : QWidget(parent)
{
    this->setStyleSheet("background-color: #ffffff");

    // 系统配置
    ConfigManager *cfg = ConfigManager::instance();
    // 加载车辆类型
    vehicleType = cfg->vehicleType();
    // 加载图片
    QString resourceFolder = cfg->resourceFolder();
    // 如果相对路径不是以 / 结尾则需要添加
    if (!resourceFolder.endsWith("/"))
    {
        resourceFolder += "/";
    }
    resourceFolder += MODEL_PNG;
    m_agvImage.load(resourceFolder);

    // 如果图片加载失败，生成一个灰色的占位图，防止程序崩溃
    if (m_agvImage.isNull())
    {
        QImage dummy(200, 300, QImage::Format_ARGB32);
        dummy.fill(Qt::gray);
        m_agvImage = QPixmap::fromImage(dummy);
    }

    // 初始化默认状态
    m_cargoState = 0; // 默认无货

    // 防撞条默认未触发 (false)
    m_bumperTop = false;
    m_bumperBottom = false;
    m_bumperLeft = false;
    m_bumperRight = false;

    // 雷达默认绿色 (Normal)
    m_radarStates[SensorZone::Top] = SensorState::Normal;
    m_radarStates[SensorZone::Bottom] = SensorState::Normal;
    m_radarStates[SensorZone::TopLeft] = SensorState::Normal;
    m_radarStates[SensorZone::BottomLeft] = SensorState::Normal;
    m_radarStates[SensorZone::TopRight] = SensorState::Normal;
    m_radarStates[SensorZone::BottomRight] = SensorState::Normal;

    // 初始化定时器
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(UPDATE_INTERVAL_MS);
    connect(m_updateTimer, &QTimer::timeout, this, &VehicleInfoWidget::updateUi);
    m_updateTimer->start();
}

VehicleInfoWidget::~VehicleInfoWidget()
{
    if (m_updateTimer->isActive())
    {
        m_updateTimer->stop();
    }
}

// 直接保存 int 状态
void VehicleInfoWidget::setCargoState(int cargoState)
{
    m_cargoState = cargoState;
}

void VehicleInfoWidget::setBumperState(bool top, bool bottom, bool left, bool right)
{
    // 更新内部成员变量
    m_bumperTop = top;
    m_bumperBottom = bottom;
    m_bumperLeft = left;
    m_bumperRight = right;
}

void VehicleInfoWidget::setRadarState(SensorZone zone, SensorState state)
{
    // 更新对应区域的状态
    // 如果 key 不存在会自动插入，如果存在则更新 value
    m_radarStates[zone] = state;
}

void VehicleInfoWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // --- 尺寸参数定义 ---
    int agvDisplayHeight = 300; // 固定高度 300
    // 根据原始宽高比计算显示宽度
    double aspectRatio = (double)m_agvImage.width() / m_agvImage.height();
    int agvDisplayWidth = (int)(agvDisplayHeight * aspectRatio);

    int cx = width() / 2;
    int cy = height() / 2;

    // AGV 主体的矩形区域
    QRect agvRect(cx - agvDisplayWidth / 2, cy - agvDisplayHeight / 2, agvDisplayWidth, agvDisplayHeight);

    // 间距参数
    int gap = 5;         // 物体间的间隙
    int bumperThick = 8; // 防撞条厚度
    int radarThick = 60; // 雷达区域厚度/宽度

    // --- 绘制 AGV 本体 ---
    painter.drawPixmap(agvRect, m_agvImage);

    // --- 绘制货物 (黄色方框，高度为1/2，位于上半部分) ---
    if (m_cargoState > 0)
    {
        painter.save();

        // 设置货物颜色 (半透明黄)
        QColor cargoColor(255, 255, 0, 150);
        painter.setBrush(cargoColor);
        QPen pen(QColor(200, 200, 0), 2);
        painter.setPen(pen);

        // 计算左右货物的几何尺寸
        // 货物总高度设为车身的一半
        int cargoHeight = agvRect.height() / 2;

        // 根据车型决定绘制方式
        if (vehicleType == 1)
        {
            // === 双叉模式 (统一绘制一个大方块) ===
            // 不区分左右，直接覆盖整个车身宽度
            QRect fullCargoRect(agvRect.x(), agvRect.y(), agvRect.width(), cargoHeight);

            painter.drawRect(fullCargoRect);
            // 画一个大叉
            painter.drawLine(fullCargoRect.topLeft(), fullCargoRect.bottomRight());
            painter.drawLine(fullCargoRect.topRight(), fullCargoRect.bottomLeft());
        }
        else if (vehicleType == 2)
        {
            // === 四叉模式 (保留你原有的左右区分逻辑) ===

            // 单个货物宽度
            int singleCargoWidth = (agvRect.width() - gap) / 2;

            // 左侧货物矩形 (视觉左侧)
            QRect leftCargoRect(agvRect.x(), agvRect.y(), singleCargoWidth, cargoHeight);

            // 右侧货物矩形 (视觉右侧)
            QRect rightCargoRect(agvRect.x() + singleCargoWidth + gap, agvRect.y(), singleCargoWidth, cargoHeight);

            // 根据你的逻辑：State 2 为左货(视觉左)，State 1 为右货(视觉右)
            // 绘制左货
            if (m_cargoState == 2 || m_cargoState == 3)
            {
                painter.drawRect(leftCargoRect);
                painter.drawLine(leftCargoRect.topLeft(), leftCargoRect.bottomRight());
                painter.drawLine(leftCargoRect.topRight(), leftCargoRect.bottomLeft());
            }

            // 绘制右货
            if (m_cargoState == 1 || m_cargoState == 3)
            {
                painter.drawRect(rightCargoRect);
                painter.drawLine(rightCargoRect.topLeft(), rightCargoRect.bottomRight());
                painter.drawLine(rightCargoRect.topRight(), rightCargoRect.bottomLeft());
            }
        }

        painter.restore();
    }

    // --- 绘制防撞条 (上下左右细条) ---
    // 顶部
    QRect bumperRectTop(agvRect.left(), agvRect.top() - gap - bumperThick, agvRect.width(), bumperThick);
    drawBumper(painter, bumperRectTop, m_bumperTop);

    // 底部
    QRect bumperRectBottom(agvRect.left(), agvRect.bottom() + gap, agvRect.width(), bumperThick);
    drawBumper(painter, bumperRectBottom, m_bumperBottom);

    // 左侧
    QRect bumperRectLeft(agvRect.left() - gap - bumperThick, agvRect.top(), bumperThick, agvRect.height());
    drawBumper(painter, bumperRectLeft, m_bumperLeft);

    // 右侧
    QRect bumperRectRight(agvRect.right() + gap, agvRect.top(), bumperThick, agvRect.height());
    drawBumper(painter, bumperRectRight, m_bumperRight);

    // --- 绘制避障雷达区域 (更外层的方框) ---

    // [上雷达] 对应顶部防撞条上方
    QRect radarRectTop(bumperRectTop.left(), bumperRectTop.top() - gap - radarThick, bumperRectTop.width(), radarThick);
    drawRadarBox(painter, radarRectTop, m_radarStates[SensorZone::Top]);

    // [下雷达] 对应底部防撞条下方
    QRect radarRectBottom(bumperRectBottom.left(), bumperRectBottom.bottom() + gap, bumperRectBottom.width(), radarThick);
    drawRadarBox(painter, radarRectBottom, m_radarStates[SensorZone::Bottom]);

    // [左侧雷达] 需要分为"左前"和"左后"两部分
    // 计算左侧总高度的一半，减去中间一点小间隙
    int sideRadarHeight = (bumperRectLeft.height() - gap) / 2;

    // 左前 (上方)
    QRect radarRectLeftFront(bumperRectLeft.left() - gap - radarThick, bumperRectLeft.top(), radarThick, sideRadarHeight);
    drawRadarBox(painter, radarRectLeftFront, m_radarStates[SensorZone::TopLeft]);

    // 左后 (下方)
    QRect radarRectLeftBack(bumperRectLeft.left() - gap - radarThick, bumperRectLeft.top() + sideRadarHeight + gap, radarThick, sideRadarHeight);
    drawRadarBox(painter, radarRectLeftBack, m_radarStates[SensorZone::BottomLeft]);

    // [右侧雷达] 分为"右前"和"右后"
    // 右前 (上方)
    QRect radarRectRightFront(bumperRectRight.right() + gap, bumperRectRight.top(), radarThick, sideRadarHeight);
    drawRadarBox(painter, radarRectRightFront, m_radarStates[SensorZone::TopRight]);

    // 右后 (下方)
    QRect radarRectRightBack(bumperRectRight.right() + gap, bumperRectRight.top() + sideRadarHeight + gap, radarThick, sideRadarHeight);
    drawRadarBox(painter, radarRectRightBack, m_radarStates[SensorZone::BottomRight]);
}

// 辅助：获取状态颜色
QColor VehicleInfoWidget::getStateColor(SensorState state)
{
    switch (state)
    {
    case SensorState::Normal:
        return lightGreen; // 亮绿
    case SensorState::Warning:
        return lightYellow; // 黄
    case SensorState::Alarm:
        return lightRed; // 红
    default:
        return Qt::gray;
    }
}

// 辅助：绘制防撞条 (绿色正常，红色触发)
void VehicleInfoWidget::drawBumper(QPainter &painter, const QRect &rect, bool isTriggered)
{
    painter.save();
    QColor color = isTriggered ? lightRed : lightGreen;
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawRect(rect);
    painter.restore();
}

// 辅助：绘制雷达块
void VehicleInfoWidget::drawRadarBox(QPainter &painter, const QRect &rect, SensorState state)
{
    painter.save();
    QColor color = getStateColor(state);
    painter.setBrush(color);
    painter.setPen(Qt::white); // 加个白边看起来更有层次感
    painter.drawRect(rect);
    painter.restore();
}

void VehicleInfoWidget::updateUi()
{
    // 获取 AgvData 实例
    AgvData *agvData = AgvData::instance();
    // 处理货物
    int goodsState = agvData->goodsState().value;
    setCargoState(goodsState);
    // 处理防撞条
    int bumpBack = agvData->bumpBack().value;
    int bumpFront = agvData->bumpFront().value;
    int bumpRight = agvData->bumpRight().value;
    int bumpLeft = agvData->bumpLeft().value;
    setBumperState(static_cast<bool>(bumpBack), static_cast<bool>(bumpFront), static_cast<bool>(bumpRight), static_cast<bool>(bumpLeft));
    // 处理避障区域
    int backArea = agvData->backArea().value;
    int backRight = agvData->backRight().value;
    int backLeft = agvData->backLeft().value;
    int frontArea = agvData->frontArea().value;
    int frontRight = agvData->frontRight().value;
    int frontLeft = agvData->frontLeft().value;
    handleArea(backArea, backRight, backLeft, frontArea, frontLeft, frontRight);
    // 更新完毕后统一触发重绘
    update();
}

void VehicleInfoWidget::handleArea(int backArea, int backRight, int backLeft, int frontArea, int frontLeft, int frontRight)
{
    SensorState topState = parseRadarState(backArea);
    setRadarState(SensorZone::Top, topState);
    SensorState bottomState = parseRadarState(frontArea);
    setRadarState(SensorZone::Bottom, bottomState);
    SensorState topLeftState = parseRadarState(backRight);
    setRadarState(SensorZone::TopLeft, topLeftState);
    SensorState bottomLeftState = parseRadarState(frontRight);
    setRadarState(SensorZone::BottomLeft, bottomLeftState);
    SensorState topRightState = parseRadarState(backLeft);
    setRadarState(SensorZone::TopRight, topRightState);
    SensorState bottomRightState = parseRadarState(frontLeft);
    setRadarState(SensorZone::BottomRight, bottomRightState);
}

SensorState VehicleInfoWidget::parseRadarState(int state)
{
    SensorState result = SensorState::Normal;

    switch (state)
    {
    case 0:
        break;

    case 1:
        result = SensorState::Warning;
        break;

    case 2:
        result = SensorState::Alarm;
        break;

    default:
        break;
    }

    return result;
}