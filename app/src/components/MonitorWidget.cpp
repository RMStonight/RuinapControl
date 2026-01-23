#include "components/MonitorWidget.h"
#include "utils/RosBridgeClient.h"
#include "utils/ConfigManager.h"
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QLineF>

MonitorWidget::MonitorWidget(QWidget *parent) : QWidget(parent)
{
    this->setStyleSheet("background-color: #1E1E1E;");
    setAttribute(Qt::WA_AcceptTouchEvents);

    ConfigManager *cfg = ConfigManager::instance();

    // 直接加载本地地图
    // 假设你的图片在资源文件中，且你知道原点是 (-10, -10)，分辨率 0.05
    QString mapUrl = cfg->mapPngFolder();
    if (!mapUrl.endsWith("/"))
    {
        mapUrl += "/";
    }
    loadLocalMap(mapUrl + "2.png", 0.02, 0, 0);

    // 创建线程
    m_rosThread = new QThread(this);

    // 创建 Client (注意：不能传 this 作为 parent，否则无法移动线程)
    m_rosClient = new RosBridgeClient();

    // 移动到子线程
    m_rosClient->moveToThread(m_rosThread);

    // 连接信号槽 (UI 更新逻辑不变)
    connect(m_rosClient, &RosBridgeClient::scanReceived, this, &MonitorWidget::updateScan);

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

// 载入 map 的本地 png 格式文件
void MonitorWidget::loadLocalMap(const QString &imagePath, double resolution, double originX, double originY)
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

    m_mapResolution = resolution;
    m_mapOriginX = originX;
    m_mapOriginY = originY;
    m_hasMap = true;

    // 3. 触发重绘
    update();

    qDebug() << "Local map loaded. Size:" << img.size() << "Res:" << resolution << "Origin:" << originX << originY;
}

// 重载 paintEvent 方法，实际上是场景重绘
void MonitorWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 设置坐标系
    // 将原点移动到屏幕中心 + 偏移量
    painter.translate(m_offset);
    // 缩放
    painter.scale(m_scale, m_scale);

    // --- 绘制图层 ---

    // 绘制网格 (辅助线)
    painter.setPen(QPen(QColor(60, 60, 60), 0)); // 极细线
    // 画一个简单的十字准星代表 (0,0)
    painter.drawLine(-10, 0, 10, 0);
    painter.drawLine(0, -10, 0, 10);

    // 绘制地图
    if (m_hasMap)
    {
        painter.save();
        // ROS 坐标系到 Qt 图像坐标系的变换
        // 假设 mapOrigin 是 (-10, -10)，分辨率 0.05
        // 图片左上角的世界坐标 = origin

        // 缩放：像素 -> 米
        double worldWidth = m_mapPixmap.width() * m_mapResolution;
        double worldHeight = m_mapPixmap.height() * m_mapResolution;

        // 移动到地图原点
        // 注意：这里需要仔细调试坐标系翻转问题，暂时按标准 2D 逻辑写
        // painter.scale(1, -1); // 翻转 Y 轴，让向上为正
        painter.translate(m_mapOriginX, m_mapOriginY);

        // 绘制图片 (将图片缩放到世界尺寸)
        // QPainter 绘制图片是以像素为单位，所以要反向缩放回来或者直接用 drawImage 的 target rect
        QRectF targetRect(0, -worldHeight, worldWidth, worldHeight);
        painter.drawPixmap(targetRect, m_mapPixmap, m_mapPixmap.rect());

        painter.restore();
    }

    // 绘制 AGV 自身图标
    painter.setBrush(Qt::green);
    // 画个三角形代表 AGV
    QPolygonF agvShape;
    agvShape << QPointF(0.2, 0) << QPointF(-0.1, 0.1) << QPointF(-0.1, -0.1);
    painter.drawPolygon(agvShape);

    // 绘制雷达点
    if (!m_scanLines.isEmpty())
    {
        QPen pen(Qt::red);
        pen.setCosmetic(true);
        pen.setWidth(5);
        pen.setCapStyle(Qt::RoundCap);

        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        // 【极速】直接绘制缓存好的线，零计算量
        painter.drawLines(m_scanLines);
    }
}

// 鼠标交互：平移
void MonitorWidget::mousePressEvent(QMouseEvent *event)
{
    // 如果正在进行触摸操作，忽略鼠标事件，防止逻辑冲突
    if (m_touchActive)
        return;

    if (event->button() == Qt::LeftButton)
    {
        m_lastMousePos = event->localPos();
    }
}

void MonitorWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_touchActive)
        return;

    if (event->buttons() & Qt::LeftButton)
    {
        QPointF currentPos = event->localPos();
        // 计算浮点数差值
        QPointF delta = currentPos - m_lastMousePos;
        m_offset += delta;
        m_lastMousePos = currentPos;
        update();
    }
}

// 鼠标交互：缩放
void MonitorWidget::wheelEvent(QWheelEvent *event)
{
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
    // 拦截触摸事件
    switch (event->type())
    {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        // 将通用事件转为触摸事件并处理
        handleTouchEvent(static_cast<QTouchEvent *>(event));
        return true; // 告诉 Qt 我们已经处理了这个事件
    default:
        // 其他事件（如鼠标、绘图等）交给父类默认处理
        return QWidget::event(event);
    }
}

// 原来的 touchEvent 改名为 handleTouchEvent，逻辑完全不变
void MonitorWidget::handleTouchEvent(QTouchEvent *event)
{
    // 获取触摸点列表
    const QList<QTouchEvent::TouchPoint> &points = event->touchPoints();

    // 状态标记
    if (event->type() == QEvent::TouchBegin)
    {
        m_touchActive = true;
    }
    else if (event->type() == QEvent::TouchEnd || event->type() == QEvent::TouchCancel)
    {
        m_touchActive = false;
    }

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