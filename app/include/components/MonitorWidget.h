#ifndef MONITORWIDGET_H
#define MONITORWIDGET_H

#include <QWidget>
#include <QImage>
#include <QVector>
#include <QPointF>
#include <QTouchEvent>

class RosBridgeClient;

class MonitorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MonitorWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;      // 缩放
    void mousePressEvent(QMouseEvent *event) override; // 拖拽平移
    void mouseMoveEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override;

private slots:
    void updateMap(const QImage &img, double x, double y, double res);
    void updateScan(const QVector<QPointF> &points);

private:
    void handleTouchEvent(QTouchEvent *event);

private:
    RosBridgeClient *m_rosClient;

    // 数据缓存
    QImage m_mapImage;
    double m_mapOriginX = 0;
    double m_mapOriginY = 0;
    double m_mapResolution = 0.05;
    bool m_hasMap = false;

    QVector<QPointF> m_scanPoints;

    // 视图变换参数 (缩放、平移)
    double m_scale = 20.0;                // 初始缩放：1米 = 20像素
    QPointF m_offset = QPointF(400, 300); // 视图中心偏移
    QPointF m_lastMousePos;

    // 用于触摸逻辑的标志位，防止触摸和鼠标事件冲突
    bool m_touchActive = false;
};

#endif // MONITORWIDGET_H