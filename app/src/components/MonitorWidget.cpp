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
    // 黑色背景，显专业
    this->setStyleSheet("background-color: #1E1E1E;");

    // 开启触摸事件支持，这是工控机触屏的关键
    setAttribute(Qt::WA_AcceptTouchEvents);

    // 初始化 ROS 客户端
    m_rosClient = new RosBridgeClient(this);

    connect(m_rosClient, &RosBridgeClient::mapReceived, this, &MonitorWidget::updateMap);
    connect(m_rosClient, &RosBridgeClient::scanReceived, this, &MonitorWidget::updateScan);

    // 从配置读取 IP 并连接
    QString ip = ConfigManager::instance()->rosBridgeIp();
    int port = ConfigManager::instance()->rosBridgePort(); // 假设 rosbridge 也是这个端口，或者在 Config 里加个 websocket 端口
    // 默认 rosbridge 端口通常是 9090
    QString url = QString("ws://%1:%2").arg(ip).arg(port);

    m_rosClient->connectToRos(url);
}

void MonitorWidget::updateMap(const QImage &img, double x, double y, double res)
{
    m_mapImage = img;
    m_mapOriginX = x;
    m_mapOriginY = y;
    m_mapResolution = res;
    m_hasMap = true;
    update(); // 触发重绘
    qDebug() << "update map";
}

void MonitorWidget::updateScan(const QVector<QPointF> &points)
{
    // [DEBUG] 偶尔打印一下 scan 数量，防止刷屏
    // static int counter = 0;
    // if (counter++ % 20 == 0) {
    //     qDebug() << "UI: Scan Received, count:" << points.size();
    // }

    m_scanPoints = points;
    update(); // 触发重绘
    
}

void MonitorWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

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
        double worldWidth = m_mapImage.width() * m_mapResolution;
        double worldHeight = m_mapImage.height() * m_mapResolution;

        // 移动到地图原点
        // 注意：这里需要仔细调试坐标系翻转问题，暂时按标准 2D 逻辑写
        painter.scale(1, -1); // 翻转 Y 轴，让向上为正
        painter.translate(m_mapOriginX, m_mapOriginY);

        // 绘制图片 (将图片缩放到世界尺寸)
        // QPainter 绘制图片是以像素为单位，所以要反向缩放回来或者直接用 drawImage 的 target rect
        QRectF targetRect(0, 0, worldWidth, worldHeight);
        // source rect 保持默认
        painter.drawImage(targetRect, m_mapImage);

        painter.restore();
    }

    // 绘制雷达点 (LaserScan)
    if (!m_scanPoints.isEmpty())
    {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 0, 0, 200)); // 半透明红色

        for (const QPointF &p : m_scanPoints)
        {
            // 绘制半径为 0.05米 的小圆点
            painter.drawEllipse(p, 0.05, 0.05);
        }
    }

    // 绘制 AGV 自身图标
    painter.setBrush(Qt::green);
    // 画个三角形代表 AGV
    QPolygonF agvShape;
    agvShape << QPointF(0.2, 0) << QPointF(-0.1, 0.1) << QPointF(-0.1, -0.1);
    painter.drawPolygon(agvShape);
}

// 鼠标交互：平移
void MonitorWidget::mousePressEvent(QMouseEvent *event)
{
    // 如果正在进行触摸操作，忽略鼠标事件，防止逻辑冲突
    if (m_touchActive) return;

    if (event->button() == Qt::LeftButton)
    {
        m_lastMousePos = event->localPos();
    }
}

void MonitorWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_touchActive) return;

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
    switch (event->type()) {
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
    if (event->type() == QEvent::TouchBegin) {
        m_touchActive = true;
    } else if (event->type() == QEvent::TouchEnd || event->type() == QEvent::TouchCancel) {
        m_touchActive = false;
    }

    // --- 单指平移 ---
    if (points.count() == 1)
    {

        QPointF currentPos = points.first().pos();
        QPointF lastPos = points.first().lastPos();

        if (event->type() == QEvent::TouchUpdate) {
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
            if (newScale < 1.0) newScale = 1.0;
            if (newScale > 500.0) newScale = 500.0;

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