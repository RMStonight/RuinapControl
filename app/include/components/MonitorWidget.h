#ifndef MONITORWIDGET_H
#define MONITORWIDGET_H

#include "BaseDisplayWidget.h"
#include <QImage>
#include <QVector>
#include <QPointF>
#include <QTouchEvent>
#include <QLabel>
#include <QPushButton>
#include <QMap>
#include <QJsonObject>
#include "LogManager.h"
#include "AgvData.h"

// 前向声明，减少头文件耦合
class BaseLayer;
class GridLayer;
class MapLayer;
class AgvLayer;
class PointPathLayer;
class PointCloudLayer;
class RelocationLayer;
class FixedRelocationLayer;
class MapDataManager;
class MonitorInteractionHandler;
class RelocationController;
class ConfigManager;
class AgvData;

class MonitorWidget : public BaseDisplayWidget
{
    Q_OBJECT
    // 允许交互处理器直接操作位姿变量 m_scale, m_offset 等
    friend class MonitorInteractionHandler;
    friend class RelocationController;

public:
    explicit MonitorWidget(QWidget *parent = nullptr);
    ~MonitorWidget();

    // 地图与视图控制接口
    void loadLocalMap(const QString &imagePath, double originX, double originY);
    void loadMapJson(const QString &path);
    void centerOnAgv();
    void setMapId(int id);

protected:
    // Qt 事件重写
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

signals:
    void pointClicked(int id);
    void baseIniPose(const QPointF &pos, double angle);

public slots:
    void handleMapIdChanged(int mapId);

private slots:
    // 业务回调与按钮逻辑
    void updatePointCloud(const QVector<QPointF> &points);
    void updateAgvState(const QVector<int> &agvState);
    // 响应固定重定位的返回数据
    void handleFixedRelocation(bool state, int x, int y, int angle);

private:
    // 内部私有辅助逻辑
    void handleMapName(int mapId);
    void handleMapJsonName(int mapId);
    bool isInDrawingArea(const QPointF &pos);
    void checkPointClick(const QPointF &screenPos);

private:
    AgvData *agvData = AgvData::instance();
    LogManager *logger = &LogManager::instance(); // 日志管理器

    // 剥离出的功能模块指针
    MapDataManager *m_mapDataManager = nullptr;
    MonitorInteractionHandler *m_interactionHandler = nullptr;
    RelocationController *m_reloController = nullptr;

    // UI 组件
    QLabel *m_mapIdLabel = nullptr;
    QPushButton *m_reloBtn = nullptr;
    QPushButton *m_switchBtn = nullptr;
    QPushButton *m_confirmBtn = nullptr;
    QPushButton *m_cancelBtn = nullptr;

    // 视图变换变量
    double m_scale = 50.0;
    QPointF m_offset = QPointF(400, 300);

    // 地图资源状态
    QPixmap m_mapPixmap;
    double m_mapOriginX = 0;
    double m_mapOriginY = 0;
    double m_mapResolution = 0.05;
    QString m_mapName = "";
    QString m_mapJsonName = "";

    // AGV 状态缓存
    int m_agvX = 0;
    int m_agvY = 0;
    int m_agvAngle = 0;

    // 交互状态
    bool m_touchActive = false;
    bool m_isRelocating = false;

    // 图层管理
    QList<BaseLayer *> m_layers;
    MapLayer *m_mapLayer = nullptr;
    AgvLayer *m_agvLayer = nullptr;
    PointPathLayer *m_pointPathLayer = nullptr;
    PointCloudLayer *m_pointCloudLayer = nullptr;
    RelocationLayer *m_reloLayer = nullptr;
    FixedRelocationLayer *m_fixedReloLayer = nullptr;
};

#endif // MONITORWIDGET_H