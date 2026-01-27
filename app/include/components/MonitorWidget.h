#ifndef MONITORWIDGET_H
#define MONITORWIDGET_H

#include "BaseDisplayWidget.h" // 1. 包含基类
#include <QImage>
#include <QVector>
#include <QPointF>
#include <QTouchEvent>
#include <QThread>
#include <QLabel>

class RosBridgeClient;

class MonitorWidget : public BaseDisplayWidget // 2. 继承基类
{
    Q_OBJECT
public:
    explicit MonitorWidget(QWidget *parent = nullptr);
    ~MonitorWidget();

    void loadLocalMap(const QString &imagePath, double originX, double originY);
    void centerOnAgv(); 
    void setMapId(int id);
    // setSharedOptionalInfo 已由基类实现

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;      
    void mousePressEvent(QMouseEvent *event) override; 
    void mouseMoveEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    // resizeEvent 已由基类处理

private slots:
    void updateScan(const QVector<QPointF> &points);
    void handleMapName(int mapId);
    void updateAgvState(const QVector<int> &agvState);

public slots:
    void handleMapIdChanged(int mapId);

private:
    void handleTouchEvent(QTouchEvent *event);
    bool isInDrawingArea(const QPointF &pos); 

private:
    RosBridgeClient *m_rosClient;
    QThread *m_rosThread;

    // OptionalInfoWidget *m_currentSideBar; // 已移动到基类
    
    QLabel *m_mapIdLabel;
    QPixmap m_mapPixmap;
    double m_mapOriginX = 0;
    double m_mapOriginY = 0;
    double m_mapResolution = 0.05;
    bool m_hasMap = false;
    QString mapUrl;
    QString m_mapName = "";
    int m_agvX = 0;
    int m_agvY = 0;
    int m_agvAngle = 0;

    QVector<QPointF> m_scanPoints;
    QVector<QLineF> m_scanLines;

    double m_scale = 50.0;
    QPointF m_offset = QPointF(400, 300);
    QPointF m_lastMousePos;
    bool m_touchActive = false;
};

#endif // MONITORWIDGET_H