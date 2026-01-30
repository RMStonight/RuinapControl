#ifndef MONITORWIDGET_H
#define MONITORWIDGET_H

#include "BaseDisplayWidget.h" // 1. 包含基类
#include <QImage>
#include <QVector>
#include <QPointF>
#include <QTouchEvent>
#include <QLabel>
#include "layers/BaseLayer.h"
#include "layers/GridLayer.h"
#include "layers/MapLayer.h"
#include "layers/AgvLayer.h"
#include "layers/PointPathLayer.h"
#include "layers/PointCloudLayer.h"
#include "utils/ConfigManager.h"

class MonitorWidget : public BaseDisplayWidget // 2. 继承基类
{
    Q_OBJECT
public:
    explicit MonitorWidget(QWidget *parent = nullptr);
    ~MonitorWidget();

    void loadLocalMap(const QString &imagePath, double originX, double originY);
    void loadMapJson(const QString &path);
    void centerOnAgv();
    void setMapId(int id);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

signals:
    void pointClicked(int id); // 点击点位时发出的信号

private slots:
    void updatePointCloud(const QVector<QPointF> &points);
    void handleMapName(int mapId);
    void handleMapJsonName(int mapId);
    void updateAgvState(const QVector<int> &agvState);

public slots:
    void handleMapIdChanged(int mapId);

private:
    void handleTouchEvent(QTouchEvent *event);
    bool isInDrawingArea(const QPointF &pos);
    void checkPointClick(const QPointF &screenPos);

private:
    ConfigManager *cfg = ConfigManager::instance();

    QLabel *m_mapIdLabel;
    QPixmap m_mapPixmap;
    double m_mapOriginX = 0;
    double m_mapOriginY = 0;
    double m_mapResolution = 0.05;
    bool m_hasMap = false;
    QString m_mapName = "";
    QString m_mapJsonName = "";
    int m_agvX = 0;
    int m_agvY = 0;
    int m_agvAngle = 0;

    QVector<QPointF> m_pointCloud;

    double m_scale = 50.0;
    QPointF m_offset = QPointF(400, 300);
    QPointF m_lastMousePos;
    bool m_touchActive = false;

    QList<BaseLayer *> m_layers;
    // 为了方便传递数据，保留具体的指针引用
    MapLayer *m_mapLayer;
    RobotLayer *m_robotLayer;
    PointPathLayer *m_pointPathLayer;
    PointCloudLayer *m_pointCloudLayer;

    // 缓存点位全局变量：Key 为点位 ID，Value 为该点位的完整 JSON 对象
    QMap<int, QJsonObject> m_pointMap;
};

#endif // MONITORWIDGET_H