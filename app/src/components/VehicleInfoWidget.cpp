#include "VehicleInfoWidget.h"
#include <QPainter>
#include "utils/AgvData.h"
#include "utils/ConfigManager.h"

VehicleInfoWidget::VehicleInfoWidget(QWidget *parent) : BaseDisplayWidget(parent)
{
    this->setStyleSheet("background-color: #ffffff");

    // 系统配置
    ConfigManager *cfg = ConfigManager::instance();
    vehicleType = cfg->vehicleType(); // 加载车辆类型

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

    // 利用基类获取绘图区域
    int leftSectionWidth = getDrawingWidth();
    QRect leftRect(0, 0, leftSectionWidth, height());

    // 2. 找到左侧区域的中心点
    int cx = leftRect.center().x();
    int cy = leftRect.center().y();

    // 3. 动态计算显示尺寸
    // AGV 高度占左侧区域高度的 75%
    int agvDisplayHeight = (int)(leftRect.height() * 0.75);

    // 限制最大高度 (可根据需要调整)
    if (agvDisplayHeight > 600)
        agvDisplayHeight = 600;

    double aspectRatio = (double)m_agvImage.width() / m_agvImage.height();
    int agvDisplayWidth = (int)(agvDisplayHeight * aspectRatio);

    // [宽度校验]：确保 AGV 宽度不会撑爆左侧区域 (留20px边距)
    if (agvDisplayWidth > (leftRect.width() - 20))
    {
        agvDisplayWidth = leftRect.width() - 20;
        agvDisplayHeight = (int)(agvDisplayWidth / aspectRatio);
    }

    // AGV 主体的矩形区域
    QRect agvRect(cx - agvDisplayWidth / 2, cy - agvDisplayHeight / 2, agvDisplayWidth, agvDisplayHeight);

    // 间距参数 (根据 AGV 大小稍微动态化一点可能更好，这里暂时维持固定或微调)
    int gap = 5;
    int bumperThick = 8;
    int radarThick = 40; // 稍微调小一点，避免在分屏后显得太大覆盖出去

    // --- 下面是原有的绘制逻辑，基本无需改动，直接依赖 agvRect 即可 ---

    // 绘制 AGV 本体
    painter.drawPixmap(agvRect, m_agvImage);

    // 绘制货物
    if (m_cargoState > 0)
    {
        painter.save();
        QColor cargoColor(255, 255, 0, 150);
        painter.setBrush(cargoColor);
        QPen pen(QColor(200, 200, 0), 2);
        painter.setPen(pen);

        int cargoHeight = agvRect.height() / 2;

        if (vehicleType == 1)
        {
            QRect fullCargoRect(agvRect.x(), agvRect.y(), agvRect.width(), cargoHeight);
            painter.drawRect(fullCargoRect);
            painter.drawLine(fullCargoRect.topLeft(), fullCargoRect.bottomRight());
            painter.drawLine(fullCargoRect.topRight(), fullCargoRect.bottomLeft());
        }
        else if (vehicleType == 2)
        {
            int singleCargoWidth = (agvRect.width() - gap) / 2;
            QRect leftCargoRect(agvRect.x(), agvRect.y(), singleCargoWidth, cargoHeight);
            QRect rightCargoRect(agvRect.x() + singleCargoWidth + gap, agvRect.y(), singleCargoWidth, cargoHeight);

            if (m_cargoState == 2 || m_cargoState == 3)
            {
                painter.drawRect(leftCargoRect);
                painter.drawLine(leftCargoRect.topLeft(), leftCargoRect.bottomRight());
                painter.drawLine(leftCargoRect.topRight(), leftCargoRect.bottomLeft());
            }
            if (m_cargoState == 1 || m_cargoState == 3)
            {
                painter.drawRect(rightCargoRect);
                painter.drawLine(rightCargoRect.topLeft(), rightCargoRect.bottomRight());
                painter.drawLine(rightCargoRect.topRight(), rightCargoRect.bottomLeft());
            }
        }
        painter.restore();
    }

    // 绘制防撞条
    QRect bumperRectTop(agvRect.left(), agvRect.top() - gap - bumperThick, agvRect.width(), bumperThick);
    drawBumper(painter, bumperRectTop, m_bumperTop);

    QRect bumperRectBottom(agvRect.left(), agvRect.bottom() + gap, agvRect.width(), bumperThick);
    drawBumper(painter, bumperRectBottom, m_bumperBottom);

    QRect bumperRectLeft(agvRect.left() - gap - bumperThick, agvRect.top(), bumperThick, agvRect.height());
    drawBumper(painter, bumperRectLeft, m_bumperLeft);

    QRect bumperRectRight(agvRect.right() + gap, agvRect.top(), bumperThick, agvRect.height());
    drawBumper(painter, bumperRectRight, m_bumperRight);

    // 绘制雷达区域
    // 注意：如果 AGV 变大了，radarThick 可能需要适当调整，或者确保它不会画到右侧区域去。
    // 由于我们在 calculate agvDisplayHeight 时预留了 padding，一般不会越界。

    QRect radarRectTop(bumperRectTop.left(), bumperRectTop.top() - gap - radarThick, bumperRectTop.width(), radarThick);
    drawRadarBox(painter, radarRectTop, m_radarStates[SensorZone::Top]);

    QRect radarRectBottom(bumperRectBottom.left(), bumperRectBottom.bottom() + gap, bumperRectBottom.width(), radarThick);
    drawRadarBox(painter, radarRectBottom, m_radarStates[SensorZone::Bottom]);

    int sideRadarHeight = (bumperRectLeft.height() - gap) / 2;

    QRect radarRectLeftFront(bumperRectLeft.left() - gap - radarThick, bumperRectLeft.top(), radarThick, sideRadarHeight);
    drawRadarBox(painter, radarRectLeftFront, m_radarStates[SensorZone::TopLeft]);

    QRect radarRectLeftBack(bumperRectLeft.left() - gap - radarThick, bumperRectLeft.top() + sideRadarHeight + gap, radarThick, sideRadarHeight);
    drawRadarBox(painter, radarRectLeftBack, m_radarStates[SensorZone::BottomLeft]);

    QRect radarRectRightFront(bumperRectRight.right() + gap, bumperRectRight.top(), radarThick, sideRadarHeight);
    drawRadarBox(painter, radarRectRightFront, m_radarStates[SensorZone::TopRight]);

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