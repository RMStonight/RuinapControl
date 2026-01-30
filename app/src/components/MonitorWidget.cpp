#include "components/MonitorWidget.h"
#include "utils/RosBridgeClient.h"
#include "AgvData.h"
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QLineF>
#include <QtMath>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

MonitorWidget::MonitorWidget(QWidget *parent) : BaseDisplayWidget(parent)
{
    this->setStyleSheet("background-color: #ffffff;");
    setAttribute(Qt::WA_AcceptTouchEvents);

    // 初始化左上角 Label ---
    m_mapIdLabel = new QLabel(this); // 指定 this 为父对象，使其依附于当前窗口
    m_mapIdLabel->setText("地图编号: -100");

    // 设置样式：白色文字，半透明黑色背景(防止地图太亮看不清文字)，字号加大，圆角
    m_mapIdLabel->setStyleSheet(
        "color: white;"
        "font-size: 16px;"
        "font-weight: bold;"
        "background-color: rgba(0, 0, 0, 100);"
        "padding: 6px;"
        "border-radius: 4px;");

    // 根据文字内容自动调整大小
    m_mapIdLabel->adjustSize();

    // 移动到左上角 (x=10, y=10) 留出一点边距
    m_mapIdLabel->move(10, 10);

    // 地图分辨率
    m_mapResolution = cfg->mapResolution() / 1000.0;

    AgvData *agvData = AgvData::instance();

    // 链接信号
    connect(agvData, &AgvData::pointCloudDataReady, this, &MonitorWidget::updatePointCloud);
    connect(agvData, &AgvData::agvStateChanged, this, &MonitorWidget::updateAgvState);

    // 初始化图层（注意顺序：先加入的先画，在底层）
    m_mapLayer = new MapLayer();
    m_pointPathLayer = new PointPathLayer();
    m_robotLayer = new RobotLayer();
    m_pointCloudLayer = new PointCloudLayer();

    m_layers << new GridLayer() << m_mapLayer << m_pointPathLayer << m_robotLayer << m_pointCloudLayer;
}

// 析构函数中记得退出线程 (如果 MonitorWidget.cpp 没有析构函数，可以加一个)
MonitorWidget::~MonitorWidget()
{
}

// 页面载入时触发
void MonitorWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // 每次显示界面时，自动归位到小车
    centerOnAgv();

    // width() 和 height() 返回的是像素值
    // qDebug() << "MonitorWidget ShowEvent - Width:" << this->width()
    //          << "Height:" << this->height();
}

// 强制视角以 AGV 为中心
void MonitorWidget::centerOnAgv()
{
    // 3. 使用基类的 getDrawingWidth() 计算中心
    int leftSectionWidth = getDrawingWidth();

    double centerX = leftSectionWidth / 2.0;
    double centerY = this->height() / 2.0;

    double worldX = m_agvX / 1000.0;
    double worldY = -(m_agvY / 1000.0);

    double newOffsetX = centerX - (worldX * m_scale);
    double newOffsetY = centerY - (worldY * m_scale);

    m_offset = QPointF(newOffsetX, newOffsetY);
    update();
}

// 更新地图编号
void MonitorWidget::setMapId(int id)
{
    if (m_mapIdLabel)
    {
        // 使用 arg() 格式化字符串
        m_mapIdLabel->setText(QString("地图编号: %1").arg(id));

        // [重要] 文字长度改变后，必须重新计算 Label 大小，
        // 这样半透明背景框才会跟随文字自动伸缩。
        m_mapIdLabel->adjustSize();
    }
}

// 地图编号更新
void MonitorWidget::handleMapIdChanged(int mapId)
{
    setMapId(mapId);
    handleMapName(mapId);
    handleMapJsonName(mapId);
}

// 更新 scan
void MonitorWidget::updatePointCloud(const QVector<QPointF> &points)
{
    m_pointCloudLayer->updatePoints(points);
    update();
}

// 处理 mapName
void MonitorWidget::handleMapName(int mapId)
{
    // 地图路径前缀
    QString mapUrl = cfg->mapPngFolder();
    if (!mapUrl.endsWith("/"))
    {
        mapUrl += "/";
    }
    QString newMapName = QString::number(mapId) + ".png";
    // 判断是否需要切换地图
    if (m_mapName != newMapName)
    {
        loadLocalMap(mapUrl + newMapName, 0, 0);
        qDebug() << "地图切换" << m_mapName << " -> " << newMapName << ", 当前地图分辨率: " << m_mapResolution;
        m_mapName = newMapName;
    }
}

// 处理 mapName
void MonitorWidget::handleMapJsonName(int mapId)
{
    // 地图路径前缀
    QString mapJsonUrl = cfg->mapJsonFolder();
    if (!mapJsonUrl.endsWith("/"))
    {
        mapJsonUrl += "/";
    }
    QString newMapJsonName = "points_and_path_" + QString::number(mapId) + ".json";
    // 判断是否需要切换地图
    if (m_mapJsonName != newMapJsonName)
    {
        loadMapJson(mapJsonUrl + newMapJsonName);
        qDebug() << "地图切换" << m_mapJsonName << " -> " << newMapJsonName;
        m_mapJsonName = newMapJsonName;
    }
}

// 处理 agvState
void MonitorWidget::updateAgvState(const QVector<int> &agvState)
{
    m_agvX = agvState[1];
    m_agvY = agvState[2];
    m_agvAngle = agvState[3];

    m_robotLayer->updatePose(agvState[1], agvState[2], agvState[3]);

    // 触发界面重绘
    update();

    // qDebug() << "x: " << m_agvX << ", y: " << m_agvY << ", angle: " << m_agvAngle;
}

// 载入 map 的本地 json 文件
void MonitorWidget::loadMapJson(const QString &path)
{
    // 1. 读取文件内容
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "无法打开JSON文件:" << path;
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    // 2. 解析 JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError)
    {
        qWarning() << "JSON 解析错误:" << parseError.errorString();
        return;
    }

    // 3. 提取数据
    if (doc.isObject())
    {
        QJsonObject jsonObj = doc.object();

        // 假设 JSON 结构中有一个名为 "points" 的数组
        // 例如: { "points": [ {"x": 1.2, "y": 3.4}, {"x": 5.0, "y": 6.0} ] }
        if (doc.isObject())
        {
            QJsonObject jsonObj = doc.object();

            if (jsonObj.contains("point") && jsonObj["point"].isArray())
            {
                QJsonArray pointsArray = jsonObj["point"].toArray();

                // 1. 第一轮遍历：缓存点位到 m_pointMap
                m_pointMap.clear();
                for (int i = 0; i < pointsArray.size(); ++i)
                {
                    QJsonObject pObj = pointsArray[i].toObject();
                    m_pointMap.insert(pObj.value("id").toInt(), pObj);
                }

                // 2. 第二轮遍历：构造显示用的点和线
                QVector<MapPointData> pointsList;
                QVector<MapPathData> pathsList;

                for (int i = 0; i < pointsArray.size(); ++i)
                {
                    QJsonObject pointObj = pointsArray[i].toObject();
                    int startId = pointObj.value("id").toInt();
                    QPointF startPos(pointObj.value("x").toDouble() / 1000.0,
                                     pointObj.value("y").toDouble() / 1000.0);

                    // --- 处理点位数据 ---
                    MapPointData pData;
                    pData.pos = startPos;
                    pData.id = QString::number(startId);

                    bool isCharge = pointObj.value("charge").toBool();
                    bool isAct = pointObj.value("loading").toBool() || pointObj.value("unloading").toBool();

                    // --- 颜色分配逻辑 ---
                    if (isCharge)
                    {
                        pData.color = QColor(0, 120, 215, 180); // 蓝色
                    }
                    else if (isAct)
                    {
                        pData.color = QColor(215, 120, 0, 180); // 棕色
                    }
                    else
                    {
                        pData.color = QColor(255, 0, 0, 180); // 默认红
                    }

                    pointsList.append(pData);

                    // --- 处理路径数据 (targets) ---
                    if (pointObj.contains("targets") && pointObj["targets"].isArray())
                    {
                        QJsonArray targetsArray = pointObj["targets"].toArray();
                        for (int j = 0; j < targetsArray.size(); ++j)
                        {
                            QJsonObject tObj = targetsArray[j].toObject();
                            int endId = tObj.value("id").toInt();

                            // 通过缓存查找到目标点坐标
                            if (m_pointMap.contains(endId))
                            {
                                QJsonObject targetPoint = m_pointMap.value(endId);
                                MapPathData pathData;
                                pathData.start = startPos;
                                pathData.end = QPointF(targetPoint.value("x").toDouble() / 1000.0,
                                                       targetPoint.value("y").toDouble() / 1000.0);
                                pathData.type = tObj.value("type").toInt();

                                // 解析控制点 (mm -> m)
                                QJsonObject c1 = tObj.value("ctl_1").toObject();
                                pathData.ctl1 = QPointF(c1.value("x").toDouble() / 1000.0,
                                                        c1.value("y").toDouble() / 1000.0);

                                QJsonObject c2 = tObj.value("ctl_2").toObject();
                                pathData.ctl2 = QPointF(c2.value("x").toDouble() / 1000.0,
                                                        c2.value("y").toDouble() / 1000.0);

                                pathsList.append(pathData);
                            }
                        }
                    }
                }

                if (m_pointPathLayer)
                {
                    m_pointPathLayer->updateData(pointsList, pathsList);
                }
            }
            update();
        }
        else
        {
            qWarning() << "JSON 中未找到 'points' 数组";
        }
    }

    // 4. 通知重绘（如果解析出的数据影响视觉）
    update();
}

// 载入 map 的本地 png 格式文件
void MonitorWidget::loadLocalMap(const QString &imagePath, double originX, double originY)
{
    // 加载图片
    QImage img(imagePath);
    if (img.isNull())
    {
        qWarning() << "Failed to load map from:" << imagePath;
        return;
    }

    // 赋值给成员变量 (这些变量原本是在 updateMap 中赋值的)
    // 建议：直接转为 QPixmap 以优化渲染性能 (参考之前的优化建议)
    m_mapPixmap = QPixmap::fromImage(img);

    m_mapOriginX = originX;
    m_mapOriginY = originY;
    m_hasMap = true;

    // --- 同步给图层 ---
    if (m_mapLayer)
    {
        m_mapLayer->updateMap(m_mapPixmap, m_mapResolution, m_mapOriginX, m_mapOriginY);
    }

    // 3. 触发重绘
    update();

    qDebug() << "Local map loaded. Size:" << img.size() << "Origin:" << originX << originY;
}

void MonitorWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 1. 绘制背景（保持原样）
    int leftSectionWidth = getDrawingWidth();
    painter.fillRect(0, 0, leftSectionWidth, height(), QColor("#ffffff"));

    // 2. 视图变换 (View Transform) - 这一步是所有图层的“父级坐标系”
    // 确保缩放和平移逻辑与原来完全一致
    painter.translate(m_offset);
    painter.scale(m_scale, m_scale);

    // 3. 遍历图层执行绘制
    // 顺序：GridLayer -> MapLayer -> RobotLayer (AgvLayer)
    for (BaseLayer *layer : m_layers)
    {
        if (layer && layer->isVisible())
        {
            layer->draw(&painter);
        }
    }
}

// 判定坐标是否在左侧绘图区
bool MonitorWidget::isInDrawingArea(const QPointF &pos)
{
    return pos.x() < getDrawingWidth();
}

// 判断是否有点位被点击
void MonitorWidget::checkPointClick(const QPointF &screenPos)
{
    // 1. 屏幕坐标转世界坐标
    // World = (Screen - Offset) / Scale
    QPointF clickWorldPos = (screenPos - m_offset) / m_scale;

    // 注意：绘图时使用了 -y (在 centerOnAgv 等处体现)，所以这里反算 y 时需要变号
    double worldX = clickWorldPos.x();
    double worldY = -clickWorldPos.y();

    // 2. 遍历缓存的点位进行碰撞检测
    double clickRadius = m_pointPathLayer->radius;

    QMapIterator<int, QJsonObject> it(m_pointMap);
    while (it.hasNext())
    {
        it.next();
        QJsonObject obj = it.value();
        double px = obj.value("x").toDouble() / 1000.0;
        double py = obj.value("y").toDouble() / 1000.0;

        double dist = QLineF(worldX, worldY, px, py).length();
        if (dist < clickRadius)
        {
            int clickedId = it.key();
            qDebug() << "触摸点位被点击，ID:" << clickedId;
            emit pointClicked(clickedId);
            break;
        }
    }
}

// 鼠标交互：平移
void MonitorWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_touchActive)
        return;

    // --- 新增判定：如果点击位置在右侧面板区域，直接忽略 ---
    if (!isInDrawingArea(event->localPos()))
    {
        event->ignore(); // 让事件正常传递给子控件 OptionalInfoWidget
        return;
    }

    if (event->button() == Qt::LeftButton)
    {
        m_lastMousePos = event->localPos();

        checkPointClick(event->localPos());
    }
}

void MonitorWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_touchActive)
        return;

    // --- 新增判定：仅在绘图区处理移动 ---
    if (!isInDrawingArea(event->localPos()))
        return;

    if (event->buttons() & Qt::LeftButton)
    {
        QPointF currentPos = event->localPos();
        QPointF delta = currentPos - m_lastMousePos;
        m_offset += delta;
        m_lastMousePos = currentPos;
        update();
    }
}

// 鼠标交互：缩放
void MonitorWidget::wheelEvent(QWheelEvent *event)
{
    // --- 新增判定：如果鼠标悬停在右侧面板上，不触发地图缩放 ---
    if (!isInDrawingArea(event->position()))
    {
        return;
    }

    // --- 获取鼠标在屏幕上的当前位置 ---
    QPointF mousePos = QPointF(event->position());

    // --- 计算缩放前的“世界坐标” ---
    // (相对于地图原点/AGV的逻辑坐标)
    QPointF worldPosBeforeZoom = (mousePos - m_offset) / m_scale;

    // --- 计算缩放系数 ---
    double factor = 1.1;
    if (event->angleDelta().y() < 0)
        factor = 1.0 / factor;

    // 限制缩放范围
    double newScale = m_scale * factor;
    if (newScale < 1.0)
        newScale = 1.0;
    if (newScale > 500.0)
        newScale = 500.0;

    // 更新缩放
    m_scale = newScale;

    // --- 反向计算新的 Offset ---
    // 核心逻辑：保持鼠标下的世界坐标不变
    // mousePos = World * NewScale + NewOffset
    // => NewOffset = mousePos - (World * NewScale)
    m_offset = mousePos - (worldPosBeforeZoom * m_scale);

    update();
}

// 事件分发入口
bool MonitorWidget::event(QEvent *event)
{
    // 拦截触摸事件进行分发判定
    if (event->type() == QEvent::TouchBegin ||
        event->type() == QEvent::TouchUpdate ||
        event->type() == QEvent::TouchEnd ||
        event->type() == QEvent::TouchCancel)
    {
        QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);
        const QList<QTouchEvent::TouchPoint> &points = touchEvent->touchPoints();

        if (!points.isEmpty())
        {
            // --- 核心修改：判定触摸起始点 ---
            // 如果触摸点不在左侧绘图区，我们不处理它，直接交给 QWidget 默认处理
            // 这样 QWidget 会把触摸事件转换为滚动事件或直接发给 OptionalInfoWidget
            if (!isInDrawingArea(points.first().pos()))
            {
                return QWidget::event(event);
            }
        }

        // 如果在绘图区，执行原有的地图交互逻辑
        handleTouchEvent(touchEvent);
        return true;
    }

    // 其他事件（如鼠标、绘图等）交给父类默认处理
    return QWidget::event(event);
}

// 原来的 touchEvent 改名为 handleTouchEvent，逻辑完全不变
void MonitorWidget::handleTouchEvent(QTouchEvent *event)
{
    // 获取触摸点列表
    const QList<QTouchEvent::TouchPoint> &points = event->touchPoints();
    if (points.isEmpty())
        return;

    // 状态标记
    if (event->type() == QEvent::TouchBegin)
    {
        if (!isInDrawingArea(points.first().pos()))
        {
            m_touchActive = false; // 标记为非绘图区触摸
            return;
        }
        m_touchActive = true;

        if (points.count() == 1)
        {

            checkPointClick(points.first().pos());
        }
    }

    if (!m_touchActive)
        return; // 如果不是从绘图区开始的触摸，直接跳过

    // --- 单指平移 ---
    if (points.count() == 1)
    {

        QPointF currentPos = points.first().pos();
        QPointF lastPos = points.first().lastPos();

        if (event->type() == QEvent::TouchUpdate)
        {
            QPointF delta = currentPos - lastPos;
            m_offset += delta;
            update();
        }
    }
    // --- 双指缩放 + 平移 ---
    else if (points.count() == 2)
    {
        QPointF p1 = points[0].pos();
        QPointF p2 = points[1].pos();
        QPointF p1Last = points[0].lastPos();
        QPointF p2Last = points[1].lastPos();

        // 计算中心点
        QPointF currentCenter = (p1 + p2) / 2.0;
        QPointF lastCenter = (p1Last + p2Last) / 2.0;

        // 计算缩放比例
        double currentDist = QLineF(p1, p2).length();
        double lastDist = QLineF(p1Last, p2Last).length();

        if (lastDist > 0.1)
        {
            double scaleFactor = currentDist / lastDist;
            double newScale = m_scale * scaleFactor;

            // 限制范围
            if (newScale < 1.0)
                newScale = 1.0;
            if (newScale > 500.0)
                newScale = 500.0;

            // 以手势中心为锚点缩放
            // World = (Screen - Offset) / Scale
            QPointF worldPos = (currentCenter - m_offset) / m_scale;

            m_scale = newScale;

            // NewOffset = Screen - World * NewScale
            m_offset = currentCenter - (worldPos * m_scale);

            // 加上双指同时移动的偏移量
            m_offset += (currentCenter - lastCenter);

            update();
        }
    }

    event->accept();
}