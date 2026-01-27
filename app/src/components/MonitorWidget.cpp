#include "components/MonitorWidget.h"
#include "utils/RosBridgeClient.h"
#include "utils/ConfigManager.h"
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QLineF>
#include <QtMath>

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

    ConfigManager *cfg = ConfigManager::instance();

    // 直接加载本地地图
    // 假设你的图片在资源文件中，且你知道原点是 (-10, -10)，分辨率 0.05
    mapUrl = cfg->mapPngFolder();
    if (!mapUrl.endsWith("/"))
    {
        mapUrl += "/";
    }
    m_mapResolution = cfg->mapResolution() / 1000.0;

    // 创建线程
    m_rosThread = new QThread(this);

    // 创建 Client (注意：不能传 this 作为 parent，否则无法移动线程)
    m_rosClient = new RosBridgeClient();

    // 移动到子线程
    m_rosClient->moveToThread(m_rosThread);

    // 连接信号槽 (UI 更新逻辑不变)
    connect(m_rosClient, &RosBridgeClient::scanReceived, this, &MonitorWidget::updateScan);
    connect(m_rosClient, &RosBridgeClient::agvStateReceived, this, &MonitorWidget::updateAgvState);

    // 启动连接逻辑
    // 当线程启动时，调用 connectToRos。使用 QueueConnection 确保在子线程执行
    QString ip = cfg->rosBridgeIp();
    int port = cfg->rosBridgePort();
    QString url = QStringLiteral("ws://%1:%2").arg(ip).arg(port);

    connect(m_rosThread, &QThread::started, m_rosClient, [this, url]()
            { m_rosClient->connectToRos(url); });

    // 资源清理：当 Widget 销毁时，退出线程
    connect(m_rosThread, &QThread::finished, m_rosClient, &QObject::deleteLater);

    // 启动线程
    m_rosThread->start();
}

// 析构函数中记得退出线程 (如果 MonitorWidget.cpp 没有析构函数，可以加一个)
MonitorWidget::~MonitorWidget()
{
    if (m_rosThread->isRunning())
    {
        m_rosThread->quit();
        m_rosThread->wait();
    }
}

// 页面载入时触发
void MonitorWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // 每次显示界面时，自动归位到小车
    centerOnAgv();

    // width() 和 height() 返回的是像素值
    qDebug() << "MonitorWidget ShowEvent - Width:" << this->width()
             << "Height:" << this->height();
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
}

// 更新 scan
void MonitorWidget::updateScan(const QVector<QPointF> &points)
{
    m_scanPoints = points; // 保留原始数据以备不时之需

    // 【预计算】在这里生成绘制所需的 Line
    const double epsilon = 0.001;
    m_scanLines.clear();
    m_scanLines.reserve(points.size());

    for (const QPointF &p : points)
    {
        m_scanLines.append(QLineF(p, QPointF(p.x() + epsilon, p.y())));
    }

    update(); // 触发重绘
}

// 处理 mapName
void MonitorWidget::handleMapName(int mapId)
{
    QString newMapName = QString::number(mapId) + ".png";
    // 判断是否需要切换地图
    if (m_mapName != newMapName)
    {
        loadLocalMap(mapUrl + newMapName, 0, 0);
        qDebug() << "地图切换" << m_mapName << " -> " << newMapName << ", 当前地图分辨率: " << m_mapResolution;
        m_mapName = newMapName;
    }
}

// 处理 agvState
void MonitorWidget::updateAgvState(const QVector<int> &agvState)
{
    m_agvX = agvState[1];
    m_agvY = agvState[2];
    m_agvAngle = agvState[3];

    // 触发界面重绘
    update();

    // qDebug() << "x: " << m_agvX << ", y: " << m_agvY << ", angle: " << m_agvAngle;
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

    // 3. 触发重绘
    update();

    qDebug() << "Local map loaded. Size:" << img.size() << "Origin:" << originX << originY;
}

void MonitorWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 计算当前可用的左侧绘图区域宽度
    int leftSectionWidth = getDrawingWidth();
    // 绘制背景（如果地图没占满，保证左侧是黑色）
    painter.fillRect(0, 0, leftSectionWidth, height(), QColor("#ffffff"));

    // --- 1. 视图变换 (View Transform) ---
    // 将原点移动到屏幕中心 + 偏移量
    painter.translate(m_offset);
    // 缩放 (m_scale 代表 1米 对应的像素数)
    painter.scale(m_scale, m_scale);

    // --- 2. 绘制网格 (Grid) ---
    painter.save();
    painter.setPen(QPen(QColor(60, 60, 60), 0)); // 极细线
    painter.drawLine(QPointF(-0.5, 0), QPointF(0.5, 0));
    painter.drawLine(QPointF(0, -0.5), QPointF(0, 0.5));
    painter.restore();

    // --- 3. 绘制地图 (Map) ---
    if (m_hasMap)
    {
        painter.save();
        painter.translate(m_mapOriginX, m_mapOriginY);

        double worldWidth = m_mapPixmap.width() * m_mapResolution;
        double worldHeight = m_mapPixmap.height() * m_mapResolution;

        // 地图绘制 (假设地图原点在左下角，向上绘制)
        QRectF targetRect(0, -worldHeight, worldWidth, worldHeight);
        painter.drawPixmap(targetRect, m_mapPixmap, m_mapPixmap.rect());

        painter.restore();
    }

    // --- 4. 绘制机器人与雷达 (Robot & Scan) ---
    painter.save();

    // 4.1 全局位置变换 (Global Position)
    // 将画家移动到小车在地图上的位置
    double x_m = m_agvX / 1000.0;
    double y_m = m_agvY / 1000.0;
    double theta_rad = m_agvAngle / 1000.0;

    // ROS (x, y) -> Qt (x, -y)
    // 这一步将坐标系原点定在小车中心
    painter.translate(x_m, -y_m);

    // 4.2 旋转 (Rotation)
    // ROS 角度逆时针(CCW)为正 -> Qt 旋转顺时针(CW)
    // 使用负号抵消方向差异
    painter.rotate(-qRadiansToDegrees(theta_rad));

    // 4.3 局部坐标系修正 (Local Frame Flip)
    // 此时 X轴指向车头。
    // 但 ROS Y轴指向车左，Qt Y轴指向车右(屏幕下)。
    // 必须翻转 Y 轴，统一局部坐标系标准。
    painter.scale(1, -1);

    // --- 此时坐标系状态：原点在车中心，X轴向前，Y轴向左 (符合 ROS 标准) ---

    // 4.4 绘制雷达点 (Draw Scan)
    // 因为坐标系已经跟随小车变换，直接绘制局部雷达数据即可
    if (!m_scanLines.isEmpty())
    {
        QPen scanPen(Qt::red);
        scanPen.setCosmetic(true); // 保持像素宽度，不随缩放变粗
        scanPen.setWidth(2);
        painter.setPen(scanPen);
        painter.drawLines(m_scanLines);
    }

    // 4.5 绘制车体 (Draw AGV)
    QPolygonF agvShape;
    // 车头(0.3m), 左后(-0.2m, 0.2m), 右后(-0.2m, -0.2m)
    // 因为上面做了 scale(1, -1)，这里 (0.2) 的 Y 值会正确地画在“左侧”(屏幕上方)
    agvShape << QPointF(0.3, 0)
             << QPointF(-0.2, 0.2)
             << QPointF(-0.2, -0.2);

    painter.setBrush(QColor(0, 191, 255, 200)); // DeepSkyBlue
    painter.setPen(QPen(Qt::white, 0.02));
    painter.drawPolygon(agvShape);

    // 车中心点
    painter.setBrush(Qt::red);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(0, 0), 0.05, 0.05);

    painter.restore();
}

// 判定坐标是否在左侧绘图区
bool MonitorWidget::isInDrawingArea(const QPointF &pos)
{
    return pos.x() < getDrawingWidth();
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
    QPointF mousePos;

    // Qt 5: QWheelEvent 没有 localPos()，使用 posF() (如果有) 或者 pos()
    // 为了最大兼容性，我们使用 pos() 并转为 QPointF
    mousePos = QPointF(event->position());

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