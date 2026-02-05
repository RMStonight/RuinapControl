#ifndef VEHICLEINFOWIDGET_H
#define VEHICLEINFOWIDGET_H

#include <QWidget>
#include <QImage>
#include <QMap>
#include <QTimer>
#include <QHBoxLayout>
#include "BaseDisplayWidget.h"
#include "LogManager.h"

// 车体模型的命名方式
#define MODEL_PNG "model.png"

// 定义雷达/避障区域的状态
enum class SensorState
{
    Normal,  // 绿色：未触发
    Warning, // 黄色：触发减速
    Alarm    // 红色：触发避障
};

// 定义具体的传感器位置 ID
enum class SensorZone
{
    Top,
    Bottom,
    TopLeft,    // 左前
    BottomLeft, // 左后
    TopRight,   // 右前
    BottomRight // 右后
};

class VehicleInfoWidget : public BaseDisplayWidget
{
    Q_OBJECT
public:
    explicit VehicleInfoWidget(QWidget *parent = nullptr);
    ~VehicleInfoWidget();

    // 供外部调用的接口，用于更新数据并重绘界面
    void setCargoState(int hasCargo);
    void setBumperState(bool top, bool bottom, bool left, bool right); // true为触发(红)
    void setRadarState(SensorZone zone, SensorState state);

protected:
    // 核心绘制事件
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateUi();

private:
    // 日志管理器
    LogManager *logger = &LogManager::instance();

    // 车辆类型
    QPixmap m_agvImage; // 资源图片
    int vehicleType;    // 1 双叉叉车，2 四叉叉车

    // 状态数据
    int m_cargoState; // 0 无货，1 左货，2 右货，3 双货
    bool m_bumperTop, m_bumperBottom, m_bumperLeft, m_bumperRight;
    QMap<SensorZone, SensorState> m_radarStates;

    // 颜色
    QColor lightGreen = QColor(148, 222, 188);
    QColor lightYellow = QColor(255, 255, 172);
    QColor lightRed = QColor(241, 168, 159);

    // 辅助绘图函数
    QColor getStateColor(SensorState state);
    void drawBumper(QPainter &painter, const QRect &rect, bool isTriggered);
    void drawRadarBox(QPainter &painter, const QRect &rect, SensorState state);

    // 定时器更新 UI
    QTimer *m_updateTimer;
    const int UPDATE_INTERVAL_MS = 100;

    SensorState parseRadarState(int state);
    void handleArea(int backArea, int backRight, int backLeft, int frontArea, int frontLeft, int frontRight);
};

#endif // VEHICLEINFOWIDGET_H