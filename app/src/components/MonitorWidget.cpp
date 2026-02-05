#include "components/MonitorWidget.h"
#include "utils/RosBridgeClient.h"
#include "utils/ConfigManager.h"
#include "AgvData.h"
#include "monitor/MapDataManager.h"
#include "monitor/MonitorInteractionHandler.h"
#include "monitor/RelocationController.h"
#include "layers/GridLayer.h"
#include "layers/MapLayer.h"
#include "layers/AgvLayer.h"
#include "layers/PointPathLayer.h"
#include "layers/PointCloudLayer.h"
#include "layers/RelocationLayer.h"
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QLineF>
#include <QtMath>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "qdir.h"

MonitorWidget::MonitorWidget(QWidget *parent) : BaseDisplayWidget(parent)
{
    this->setStyleSheet("background-color: #ffffff;");
    setAttribute(Qt::WA_AcceptTouchEvents);

    // 初始化功能模块
    m_mapDataManager = new MapDataManager(this);
    m_interactionHandler = new MonitorInteractionHandler(this);
    m_reloController = new RelocationController(this);

    // 初始化左上角地图信息 Label
    m_mapIdLabel = new QLabel(this);
    m_mapIdLabel->setText("地图编号: -100");
    m_mapIdLabel->setStyleSheet(
        "color: white;"
        "font-size: 16px;"
        "font-weight: bold;"
        "background-color: rgba(0, 0, 0, 100);"
        "padding: 6px;"
        "border-radius: 4px;");
    m_mapIdLabel->adjustSize();
    m_mapIdLabel->move(10, 10);

    // 地图分辨率初始化
    m_mapResolution = ConfigManager::instance()->mapResolution() / 1000.0;

    // 链接业务信号
    connect(AgvData::instance(), &AgvData::pointCloudDataReady, this, &MonitorWidget::updatePointCloud);
    connect(AgvData::instance(), &AgvData::agvStateChanged, this, &MonitorWidget::updateAgvState);

    // 初始化图层
    m_mapLayer = new MapLayer();
    m_pointPathLayer = new PointPathLayer();
    m_agvLayer = new AgvLayer();
    m_pointCloudLayer = new PointCloudLayer();
    m_reloLayer = new RelocationLayer();

    m_layers << new GridLayer() << m_mapLayer << m_pointPathLayer << m_agvLayer << m_pointCloudLayer << m_reloLayer;

    // 初始化重定位按钮
    m_reloBtn = new QPushButton("重定位", this);
    m_confirmBtn = new QPushButton("确认", this);
    m_cancelBtn = new QPushButton("取消", this);

    QString baseStyle = "QPushButton { border-radius: 5px; font-weight: bold; color: white; }";
    m_reloBtn->setStyleSheet(baseStyle + "QPushButton { background-color: #0078d7; }");
    m_confirmBtn->setStyleSheet(baseStyle + "QPushButton { background-color: #28a745; }");
    m_cancelBtn->setStyleSheet(baseStyle + "QPushButton { background-color: #dc3545; }");

    m_reloBtn->setFixedSize(80, 40);
    m_confirmBtn->setFixedSize(80, 40);
    m_cancelBtn->setFixedSize(80, 40);

    m_reloBtn->move(10, 50);
    m_confirmBtn->move(10, 50);
    m_cancelBtn->move(100, 50);

    m_confirmBtn->hide();
    m_cancelBtn->hide();
    m_reloBtn->show();

    // 信号槽连接
    connect(m_reloBtn, &QPushButton::clicked, m_reloController, &RelocationController::start);
    connect(m_confirmBtn, &QPushButton::clicked, m_reloController, &RelocationController::finish);
    connect(m_cancelBtn, &QPushButton::clicked, m_reloController, &RelocationController::cancel);
    connect(this, &MonitorWidget::baseIniPose, AgvData::instance(), &AgvData::requestInitialPose);
}

MonitorWidget::~MonitorWidget()
{
}

void MonitorWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    centerOnAgv(); // 显示时自动对焦小车
}

// --- 地图与坐标控制逻辑 ---

void MonitorWidget::centerOnAgv()
{
    int leftSectionWidth = getDrawingWidth();
    double centerX = leftSectionWidth / 2.0;
    double centerY = this->height() / 2.0;

    double worldX = m_agvX / 1000.0;
    double worldY = -(m_agvY / 1000.0);

    m_offset = QPointF(centerX - (worldX * m_scale), centerY - (worldY * m_scale));
    update();
}

void MonitorWidget::setMapId(int id)
{
    if (m_mapIdLabel)
    {
        m_mapIdLabel->setText(QString("地图编号: %1").arg(id));
        m_mapIdLabel->adjustSize();
    }
}

void MonitorWidget::handleMapIdChanged(int mapId)
{
    setMapId(mapId);
    handleMapName(mapId);
    handleMapJsonName(mapId);
}

void MonitorWidget::handleMapName(int mapId)
{
    QString mapUrl = ConfigManager::instance()->mapPngFolder();

    QString newMapName = QString::number(mapId) + ".png";

    if (m_mapName != newMapName)
    {
        QString mapUrlPath = QDir(mapUrl).filePath(newMapName);
        loadLocalMap(mapUrlPath, 0, 0);
        m_mapName = newMapName;
    }
}

void MonitorWidget::handleMapJsonName(int mapId)
{
    QString mapJsonUrl = ConfigManager::instance()->mapJsonFolder();

    QString newMapJsonName = "points_and_path_" + QString::number(mapId) + ".json";
    if (m_mapJsonName != newMapJsonName)
    {
        QString mapJsonUrlPath = QDir(mapJsonUrl).filePath(newMapJsonName);
        loadMapJson(mapJsonUrlPath);
        m_mapJsonName = newMapJsonName;
    }
}

void MonitorWidget::loadMapJson(const QString &path)
{
    QVector<MapPointData> pointsList;
    QVector<MapPathData> pathsList;

    // 委托给 DataManager 处理解析
    if (m_mapDataManager->parseMapJson(path, pointsList, pathsList))
    {
        m_pointPathLayer->updateData(pointsList, pathsList);
        update();
    }
}

void MonitorWidget::loadLocalMap(const QString &imagePath, double originX, double originY)
{
    QImage img(imagePath);
    if (img.isNull())
        return;

    m_mapPixmap = QPixmap::fromImage(img);
    m_mapOriginX = originX;
    m_mapOriginY = originY;

    if (m_mapLayer)
    {
        m_mapLayer->updateMap(m_mapPixmap, m_mapResolution, m_mapOriginX, m_mapOriginY);
    }
    update();
}

void MonitorWidget::updatePointCloud(const QVector<QPointF> &points)
{
    m_pointCloudLayer->updatePoints(points);
    update();
}

void MonitorWidget::updateAgvState(const QVector<int> &agvState)
{
    if (m_isRelocating)
        return;

    m_agvX = agvState[1];
    m_agvY = agvState[2];
    m_agvAngle = agvState[3];

    m_agvLayer->updatePose(m_agvX, m_agvY, m_agvAngle);
    update();
}

// --- 事件与绘制逻辑 ---

void MonitorWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int leftSectionWidth = getDrawingWidth();
    painter.fillRect(0, 0, leftSectionWidth, height(), QColor("#ffffff"));

    // 应用交互处理器计算出的视口变换
    painter.translate(m_offset);
    painter.scale(m_scale, m_scale);

    for (BaseLayer *layer : m_layers)
    {
        if (layer && layer->isVisible())
        {
            if (m_isRelocating && layer == m_pointCloudLayer)
            {
                painter.save();
                painter.translate(m_reloLayer->pos().x(), m_reloLayer->pos().y());
                painter.rotate(-qRadiansToDegrees(m_reloLayer->getAngle()));
                m_pointCloudLayer->drawLocal(&painter);
                painter.restore();
                continue;
            }
            layer->draw(&painter);
        }
    }
}

bool MonitorWidget::isInDrawingArea(const QPointF &pos)
{
    return pos.x() < getDrawingWidth();
}

void MonitorWidget::checkPointClick(const QPointF &screenPos)
{
    QPointF clickWorldPos = (screenPos - m_offset) / m_scale;
    double worldX = clickWorldPos.x();
    double worldY = -clickWorldPos.y();

    const QMap<int, QJsonObject> &pointMap = m_mapDataManager->getPointMap();
    QMapIterator<int, QJsonObject> it(pointMap);
    while (it.hasNext())
    {
        it.next();
        QJsonObject obj = it.value();
        double px = obj.value("x").toDouble() / 1000.0;
        double py = obj.value("y").toDouble() / 1000.0;

        if (QLineF(worldX, worldY, px, py).length() < m_pointPathLayer->radius)
        {
            emit pointClicked(it.key());
            break;
        }
    }
}

// 事件转发至 InteractionHandler
void MonitorWidget::mousePressEvent(QMouseEvent *event) { m_interactionHandler->handleMousePress(event); }
void MonitorWidget::mouseMoveEvent(QMouseEvent *event) { m_interactionHandler->handleMouseMove(event); }
void MonitorWidget::mouseReleaseEvent(QMouseEvent *event) { m_interactionHandler->handleMouseRelease(event); }
void MonitorWidget::wheelEvent(QWheelEvent *event) { m_interactionHandler->handleWheel(event); }

bool MonitorWidget::event(QEvent *event)
{
    // 处理触摸事件的按钮拦截
    if (event->type() == QEvent::TouchBegin)
    {
        QTouchEvent *te = static_cast<QTouchEvent *>(event);
        if (!te->touchPoints().isEmpty())
        {
            QPoint p = te->touchPoints().first().pos().toPoint();
            bool onBtn = (m_reloBtn->isVisible() && m_reloBtn->geometry().contains(p)) ||
                         (m_confirmBtn->isVisible() && m_confirmBtn->geometry().contains(p)) ||
                         (m_cancelBtn->isVisible() && m_cancelBtn->geometry().contains(p));
            if (onBtn)
                return QWidget::event(event);
        }
    }

    if (event->type() >= QEvent::TouchBegin && event->type() <= QEvent::TouchCancel)
    {
        QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);
        if (!touchEvent->touchPoints().isEmpty() && !isInDrawingArea(touchEvent->touchPoints().first().pos()))
        {
            return QWidget::event(event);
        }
        m_interactionHandler->handleTouch(touchEvent);
        return true;
    }

    return BaseDisplayWidget::event(event);
}