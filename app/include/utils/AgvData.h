#ifndef AGVDATA_H
#define AGVDATA_H

#include "AgvAttribute.h"
#include <QObject>
#include <QMutex>
#include <QReadWriteLock>
#include <functional>
#include <QHash>
#include <QJsonObject>
#include <atomic>

// 为了让信号槽能用，建议定义别名
using AgvInt = AgvAttribute<int>;
using AgvString = AgvAttribute<QString>;

// 必须在类外注册元类型 (Qt 5/6)
Q_DECLARE_METATYPE(AgvInt)
Q_DECLARE_METATYPE(AgvString)

class AgvData : public QObject
{
    Q_OBJECT

public:
    // 获取单例实例
    static AgvData *instance();

    // --- Getters ---
    // AGVInfo
    AgvString agvErr() const;
    AgvInt agvXin1() const;
    AgvInt agvXin2() const;
    AgvInt agvXin3() const;
    AgvInt agvYout1() const;
    AgvInt agvYout2() const;
    AgvInt agvYout3() const;
    AgvInt agvId() const;
    AgvString agvName() const;
    AgvInt battery() const;
    AgvInt mapId() const;
    AgvInt slamX() const;
    AgvInt slamY() const;
    AgvInt slamAngle() const;
    AgvInt slamCov() const;
    AgvInt vX() const;
    AgvInt vY() const;
    AgvInt vAngle() const;
    AgvInt agvMode() const;
    AgvInt agvState() const;
    AgvInt light() const;
    AgvInt frontArea() const;
    AgvInt frontLeft() const;
    AgvInt frontRight() const;
    AgvInt backArea() const;
    AgvInt backLeft() const;
    AgvInt backRight() const;
    AgvInt bumpBack() const;
    AgvInt bumpFront() const;
    AgvInt bumpLeft() const;
    AgvInt bumpRight() const;
    AgvInt eStopState() const;
    AgvInt goodsState() const;
    AgvInt moveDir() const;
    AgvInt pointId() const;
    AgvInt taskAct() const;
    AgvInt taskParam() const;
    AgvString taskDescription() const;
    AgvInt runLength() const;
    AgvInt runTime() const;

    // OptionalINFO
    QJsonObject optionalInfo() const;
    AgvInt liftHeight() const;

    // AGV_TASK
    AgvInt taskState() const;
    AgvString taskId() const;
    AgvInt taskStartId() const;
    AgvInt taskStartX() const;
    AgvInt taskStartY() const;
    AgvInt taskEndId() const;
    AgvInt taskEndX() const;
    AgvInt taskEndY() const;
    AgvInt pathStartId() const;
    AgvInt pathStartX() const;
    AgvInt pathStartY() const;
    AgvInt pathEndId() const;
    AgvInt pathEndX() const;
    AgvInt pathEndY() const;
    AgvString taskErr() const;

    // TOUCH_STATE
    bool pageControl() const;
    bool taskCancel() const;
    bool taskStart() const;
    bool taskPause() const;
    bool taskResume() const;
    bool chargeCmd() const;
    int manualDir() const;
    int manualAct() const;
    int manualVx() const;
    int manualVy() const;
    // int manualVth() const;
    int iniX() const;
    int iniY() const;
    int iniW() const;
    int music() const;

    // --- Setters ---
    void setPageControl(bool value);
    void setTaskCancel(bool value);
    void setTaskStart(bool value);
    void setTaskPause(bool value);
    void setTaskResume(bool value);
    void setChargeCmd(bool value);
    void setManualDir(int value);
    void setManualAct(int value);
    void setManualVx(int value);
    void setManualVy(int value);
    // void setManualVth(int value);
    void setIniX(int value);
    void setIniY(int value);
    void setIniW(int value);
    void setMusic(int value);

    // 解析来自 Websocket 的 JSON 数据
    void parseAgvState(const QJsonObject &data);

    // 校验并获取 json 数据
    bool tryParseAgvJson(const QString &jsonStr, QJsonObject &resultObj);
    void handleAgvInfo(const QJsonObject &data);
    void handleOptionalInfo(const QJsonObject &data);
    void handleAgvTask(const QJsonObject &data);

public slots:
    // --- 数据处理接口 ---
    void parseMsg(const QString &msg);

signals:
    // --- 信号 ---

private:
    explicit AgvData(QObject *parent = nullptr);
    ~AgvData();

    // 初始化值
    void initData();

    // 定义一个解析器函数类型
    using ParserFunc = std::function<void(const QJsonObject &)>;
    // AGVInfo 映射表：Key -> 解析函数
    QHash<QString, ParserFunc> m_agvInfoParsers;
    QHash<QString, ParserFunc> m_optionalInfoParsers;
    QHash<QString, ParserFunc> m_agvTaskParsers;
    // 初始化映射表的函数
    void initParsers();

    // 成员变量
    // AGVInfo
    AgvString m_agvErr;          // agv 错误信息，Null 代表无错误
    AgvInt m_xin1;               // 单片机输入端 X01-X08， 0 红，1 绿
    AgvInt m_xin2;               // 单片机输入端 X09-X16， 0 红，1 绿
    AgvInt m_xin3;               // 单片机输入端 X17-X24， 0 红，1 绿
    AgvInt m_yout1;              // 单片机输出端 Y01-Y08， 0 红，1 绿
    AgvInt m_yout2;              // 单片机输出端 Y09-Y16， 0 红，1 绿
    AgvInt m_yout3;              // 单片机输出端 Y17-Y24， 0 红，1 绿
    AgvInt m_agvId;              // AGV 编号
    AgvString m_agvName;         // AGV 名称
    AgvInt m_battery;            // AGV 剩余电量，单位 %
    AgvInt m_mapId;              // 当前地图编号
    AgvInt m_slamX;              // slam 定位的 X 坐标，单位 mm
    AgvInt m_slamY;              // slam 定位的 Y 坐标，单位 mm
    AgvInt m_slamAngle;          // slam 定位的角度，除以 100 后单位为度
    AgvInt m_slamCov;            // slam 定位的协方差，实际值需要除以 1000
    AgvInt m_vX;                 // AGV 的 X 方向线速度，单位 mm/s
    AgvInt m_vY;                 // AGV 的 Y 方向线速度，单位 mm/s
    AgvInt m_vAngle;             // AGV 的角速度，单位 百度/s
    AgvInt m_agvMode;            // AGV 手自动状态，0 手动，1 自动
    AgvInt m_agvState;           // AGV自动模式状态，0 待命，1 自动行走，2 自动动作，3 充电中，10 暂停
    AgvInt m_light;              // 状态灯颜色，0 无色，1 红，2 绿，3 蓝，4 黄，5 紫，6 淡蓝，7 白
    AgvInt m_frontArea;          // 前避障，0 不触发，1 触发减速，2 触发停车，Null 为无该区域信号
    AgvInt m_frontLeft;          // 左前避障，0 不触发，1 触发减速，2 触发停车，Null 为无该区域信号
    AgvInt m_frontRight;         // 右前避障，0 不触发，1 触发减速，2 触发停车，Null 为无该区域信号
    AgvInt m_backArea;           // 后避障，0 不触发，1 触发减速，2 触发停车，Null 为无该区域信号
    AgvInt m_backLeft;           // 左后避障，0 不触发，1 触发减速，2 触发停车，Null 为无该区域信号
    AgvInt m_backRight;          // 右后避障，0 不触发，1 触发减速，2 触发停车，Null 为无该区域信号
    AgvInt m_bumpBack;           // 后防撞条，0 不触发，1 触发，Null 为无该区域信号
    AgvInt m_bumpFront;          // 前防撞条，0 不触发，1 触发，Null 为无该区域信号
    AgvInt m_bumpLeft;           // 左防撞条，0 不触发，1 触发，Null 为无该区域信号
    AgvInt m_bumpRight;          // 右防撞条，0 不触发，1 触发，Null 为无该区域信号
    AgvInt m_estopState;         // 急停状态，0 未急停，1 急停按下
    AgvInt m_goodsState;         // 载货状态，0无货，1单左货，2单右货，3左右货（双叉车型0123均有效，单叉或潜伏式0、3有效，Null为无货物信号）
    AgvInt m_moveDir;            // 运动方向，0 停车，1 前进，2 后退，3 左横移，4 右横移，5 逆原，6 顺原（3、4仅全向车有效）
    AgvInt m_pointId;            // AGV 当前对应地图点位号，Null代表附近无地图点位
    AgvInt m_taskAct;            // AGV 自动模式下的任务动作号
    AgvInt m_taskParam;          // AGV 自动模式下的任务动参
    AgvString m_taskDescription; // AGV 自动模式下的任务描述
    AgvInt m_runLength;          // 开机运行里程，单位 m
    AgvInt m_runTime;            // 开机运行事件，单位 s

    // OptionalINFO
    AgvInt m_liftHeight; // 举升高度，单位 mm

    // AGV_TASK
    AgvInt m_taskState;   // AGV 自动模式下的当前任务状态，0 车上无任务，1 车上有任务，2 任务已完成，3 任务取消
    AgvString m_taskId;   // AGV 自动模式下的当前任务号，若当前无任务则为 null
    AgvInt m_taskStartId; // 任务起点
    AgvInt m_taskStartX;  // 任务起点 x 坐标
    AgvInt m_taskStartY;  // 任务起点 y 坐标
    AgvInt m_taskEndId;   // 任务终点
    AgvInt m_taskEndX;    // 任务终点 x 坐标
    AgvInt m_taskEndY;    // 任务终点 y 坐标
    AgvInt m_pathStartId; // 路径起点
    AgvInt m_pathStartX;  // 路径起点 x 坐标
    AgvInt m_pathStartY;  // 路径起点 y 坐标
    AgvInt m_pathEndId;   // 路径终点
    AgvInt m_pathEndX;    // 路径终点 x 坐标
    AgvInt m_pathEndY;    // 路径终点 y 坐标
    AgvString m_taskErr;  // 任务错误信息，为 Null 代表无错误

    // TOUCH_STATE
    std::atomic<bool> m_pageControl; // 页面控制信号，0启用，1关闭
    std::atomic<bool> m_taskCancel;  // 取消任务, 0未触发, 1触发
    std::atomic<bool> m_taskStart;   // 开始任务, 0未触发, 1触发
    std::atomic<bool> m_taskPause;   // 暂停任务, 0未触发, 1触发
    std::atomic<bool> m_taskResume;  // 恢复任务, 0未触发, 1触发
    std::atomic<bool> m_chargeCmd;   // 手动充电信号，0不充电，1充电；拨码开关逻辑，自锁
    std::atomic<int> m_manualDir;    // 手动运动方向，0静止，1前进，2后退，3逆自，4顺自，5左前，6右前，7左后，8右后
    std::atomic<int> m_manualAct;    // 手动动作，0无，1上升，2下降，3左箭头，4右箭头
    std::atomic<int> m_manualVx;     // mm/s
    std::atomic<int> m_manualVy;     // mm/s
    // std::atomic<int> m_manualVth;    // 100°
    std::atomic<int> m_iniX;         // 重定位x
    std::atomic<int> m_iniY;         // 重定位y
    std::atomic<int> m_iniW;         // 重定位w
    std::atomic<int> m_music;        // 喇叭操作，0无，1切歌，2音量+，3音量-

    // OptionalInfo 需要单独备份
    QJsonObject m_optionalInfo;

    mutable QReadWriteLock m_lock;
};

#endif // AGVDATA_H