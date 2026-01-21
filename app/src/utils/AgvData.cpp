#include "AgvData.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>

// 全局静态指针
static AgvData *s_instance = nullptr;

// 辅助函数：从 JSON 对象中提取 AgvAttribute
// 假设 json 格式为: "battery": { "value": 80.5, "color": "#00FF00" }
template <typename T>
AgvAttribute<T> parseAttr(const QJsonObject &root, const QString &key)
{
    if (!root.contains(key))
        return AgvAttribute<T>(); // 返回默认

    QJsonObject obj = root[key].toObject();
    T val = QVariant(obj["value"].toVariant()).value<T>(); // 自动转换类型
    QString col = obj["color"].toString("#000000");        // 默认黑

    return AgvAttribute<T>(val, col);
}

AgvData *AgvData::instance()
{
    if (!s_instance)
    {
        // 简单单例实现 (注意：如果在多线程构造需加锁，但在主线程初始化通常安全)
        s_instance = new AgvData();
    }
    return s_instance;
}

// 初始化 AgvData
AgvData::AgvData(QObject *parent) : QObject(parent)
{
    initData();    // 初始化值
    initParsers(); // 初始化绑定
}

AgvData::~AgvData()
{
    // 程序退出时清理
    if (s_instance == this)
    {
        s_instance = nullptr;
    }
}

void AgvData::initData()
{
    // AgvInfo
    m_agvErr = AgvString("Initializing", "#000000");
    m_xin1 = AgvInt(0xff, "#000000");
    m_xin2 = AgvInt(0xff, "#000000");
    m_xin3 = AgvInt(0xff, "#000000");
    m_yout1 = AgvInt(0x00, "#000000");
    m_yout2 = AgvInt(0x00, "#000000");
    m_yout3 = AgvInt(0x00, "#000000");
    m_agvId = AgvInt(-1, "#000000");
    m_agvName = AgvString("Unconnected", "#000000");
    m_battery = AgvInt(100, "#000000");
    m_mapId = AgvInt(-1, "#000000");
    m_slamX = AgvInt(-1, "#000000");
    m_slamY = AgvInt(-1, "#000000");
    m_slamAngle = AgvInt(-1, "#000000");
    m_slamCov = AgvInt(-1, "#000000");
    m_vX = AgvInt(-1, "#000000");
    m_vY = AgvInt(-1, "#000000");
    m_vAngle = AgvInt(-1, "#000000");
    m_agvMode = AgvInt(0, "#000000");
    m_agvState = AgvInt(0, "#000000");
    m_light = AgvInt(0, "#000000");
    m_frontArea = AgvInt(2, "#000000");
    m_frontLeft = AgvInt(2, "#000000");
    m_frontRight = AgvInt(2, "#000000");
    m_backArea = AgvInt(2, "#000000");
    m_backLeft = AgvInt(2, "#000000");
    m_backRight = AgvInt(2, "#000000");
    m_bumpBack = AgvInt(1, "#000000");
    m_bumpFront = AgvInt(1, "#000000");
    m_bumpLeft = AgvInt(1, "#000000");
    m_bumpRight = AgvInt(1, "#000000");
    m_estopState = AgvInt(1, "#000000");
    m_goodsState = AgvInt(0, "#000000");
    m_moveDir = AgvInt(0, "#000000");
    m_pointId = AgvInt(0, "#000000");
    m_taskAct = AgvInt(0, "#000000");
    m_taskParam = AgvInt(0, "#000000");
    m_taskDescription = AgvString("", "#000000");
    m_runLength = AgvInt(0, "#000000");
    m_runTime = AgvInt(0, "#000000");
    // OptionalINFO
    m_liftHeight = AgvInt(0, "#000000");
    m_optionalErr = AgvString("NULL", "#000000");
    // AGV_TASK
    m_taskState = AgvInt(0, "#000000");
    m_taskId = AgvString("NULL", "#000000");
    m_taskStartId = AgvInt(0, "#000000");
    m_taskStartX = AgvInt(0, "#000000");
    m_taskStartY = AgvInt(0, "#000000");
    m_taskEndId = AgvInt(0, "#000000");
    m_taskEndX = AgvInt(0, "#000000");
    m_taskEndY = AgvInt(0, "#000000");
    m_pathStartId = AgvInt(0, "#000000");
    m_pathStartX = AgvInt(0, "#000000");
    m_pathStartY = AgvInt(0, "#000000");
    m_pathEndId = AgvInt(0, "#000000");
    m_pathEndX = AgvInt(0, "#000000");
    m_pathEndY = AgvInt(0, "#000000");
    m_taskErr = AgvString("NULL", "#000000");
}

void AgvData::initParsers()
{
    // AgvInfo
    m_agvInfoParsers["AGV_Err_Msg"] = [this](const QJsonObject &data)
    {
        m_agvErr = parseAttr<QString>(data, "AGV_Err_Msg");
    };
    m_agvInfoParsers["Xin1"] = [this](const QJsonObject &data)
    {
        m_xin1 = parseAttr<int>(data, "Xin1");
    };
    m_agvInfoParsers["Xin2"] = [this](const QJsonObject &data)
    {
        m_xin2 = parseAttr<int>(data, "Xin2");
    };
    m_agvInfoParsers["Xin3"] = [this](const QJsonObject &data)
    {
        m_xin3 = parseAttr<int>(data, "Xin3");
    };
    m_agvInfoParsers["Yout1"] = [this](const QJsonObject &data)
    {
        m_yout1 = parseAttr<int>(data, "Yout1");
    };
    m_agvInfoParsers["Yout2"] = [this](const QJsonObject &data)
    {
        m_yout2 = parseAttr<int>(data, "Yout2");
    };
    m_agvInfoParsers["Yout3"] = [this](const QJsonObject &data)
    {
        m_yout3 = parseAttr<int>(data, "Yout3");
    };
    m_agvInfoParsers["agv_id"] = [this](const QJsonObject &data)
    {
        m_agvId = parseAttr<int>(data, "agv_id");
    };
    m_agvInfoParsers["agv_name"] = [this](const QJsonObject &data)
    {
        m_agvName = parseAttr<QString>(data, "agv_name");
    };
    m_agvInfoParsers["battery"] = [this](const QJsonObject &data)
    {
        m_battery = parseAttr<int>(data, "battery");
    };
    m_agvInfoParsers["map_id"] = [this](const QJsonObject &data)
    {
        m_mapId = parseAttr<int>(data, "map_id");
    };
    m_agvInfoParsers["slam_x"] = [this](const QJsonObject &data)
    {
        m_slamX = parseAttr<int>(data, "slam_x");
    };
    m_agvInfoParsers["slam_y"] = [this](const QJsonObject &data)
    {
        m_slamY = parseAttr<int>(data, "slam_y");
    };
    m_agvInfoParsers["slam_angle"] = [this](const QJsonObject &data)
    {
        m_slamAngle = parseAttr<int>(data, "slam_angle");
    };
    m_agvInfoParsers["slam_cov"] = [this](const QJsonObject &data)
    {
        m_slamCov = parseAttr<int>(data, "slam_cov");
    };
    m_agvInfoParsers["v_x"] = [this](const QJsonObject &data)
    {
        m_vX = parseAttr<int>(data, "v_x");
    };
    m_agvInfoParsers["v_y"] = [this](const QJsonObject &data)
    {
        m_vY = parseAttr<int>(data, "v_y");
    };
    m_agvInfoParsers["v_angle"] = [this](const QJsonObject &data)
    {
        m_vAngle = parseAttr<int>(data, "v_angle");
    };
    m_agvInfoParsers["agv_mode"] = [this](const QJsonObject &data)
    {
        m_agvMode = parseAttr<int>(data, "agv_mode");
    };
    m_agvInfoParsers["agv_state"] = [this](const QJsonObject &data)
    {
        m_agvState = parseAttr<int>(data, "agv_state");
    };
    m_agvInfoParsers["light"] = [this](const QJsonObject &data)
    {
        m_light = parseAttr<int>(data, "light");
    };
    m_agvInfoParsers["front_area"] = [this](const QJsonObject &data)
    {
        m_frontArea = parseAttr<int>(data, "front_area");
    };
    m_agvInfoParsers["front_left"] = [this](const QJsonObject &data)
    {
        m_frontLeft = parseAttr<int>(data, "front_left");
    };
    m_agvInfoParsers["front_right"] = [this](const QJsonObject &data)
    {
        m_frontRight = parseAttr<int>(data, "front_right");
    };
    m_agvInfoParsers["back_area"] = [this](const QJsonObject &data)
    {
        m_backArea = parseAttr<int>(data, "back_area");
    };
    m_agvInfoParsers["back_left"] = [this](const QJsonObject &data)
    {
        m_backLeft = parseAttr<int>(data, "back_left");
    };
    m_agvInfoParsers["back_right"] = [this](const QJsonObject &data)
    {
        m_backRight = parseAttr<int>(data, "back_right");
    };
    m_agvInfoParsers["bump_back"] = [this](const QJsonObject &data)
    {
        m_bumpBack = parseAttr<int>(data, "bump_back");
    };
    m_agvInfoParsers["bump_front"] = [this](const QJsonObject &data)
    {
        m_bumpFront = parseAttr<int>(data, "bump_front");
    };
    m_agvInfoParsers["bump_left"] = [this](const QJsonObject &data)
    {
        m_bumpLeft = parseAttr<int>(data, "bump_left");
    };
    m_agvInfoParsers["bump_right"] = [this](const QJsonObject &data)
    {
        m_bumpRight = parseAttr<int>(data, "bump_right");
    };
    m_agvInfoParsers["estop_state"] = [this](const QJsonObject &data)
    {
        m_estopState = parseAttr<int>(data, "estop_state");
    };
    m_agvInfoParsers["goods_state"] = [this](const QJsonObject &data)
    {
        m_goodsState = parseAttr<int>(data, "goods_state");
    };
    m_agvInfoParsers["move_dir"] = [this](const QJsonObject &data)
    {
        m_moveDir = parseAttr<int>(data, "move_dir");
    };
    m_agvInfoParsers["point_id"] = [this](const QJsonObject &data)
    {
        m_pointId = parseAttr<int>(data, "point_id");
    };
    m_agvInfoParsers["task_act"] = [this](const QJsonObject &data)
    {
        m_taskAct = parseAttr<int>(data, "task_act");
    };
    m_agvInfoParsers["task_param"] = [this](const QJsonObject &data)
    {
        m_taskParam = parseAttr<int>(data, "task_param");
    };
    m_agvInfoParsers["task_description"] = [this](const QJsonObject &data)
    {
        m_taskDescription = parseAttr<QString>(data, "task_description");
    };
    m_agvInfoParsers["run_length"] = [this](const QJsonObject &data)
    {
        m_runLength = parseAttr<int>(data, "run_length");
    };
    m_agvInfoParsers["run_time"] = [this](const QJsonObject &data)
    {
        m_runTime = parseAttr<int>(data, "run_time");
    };
    // OptionalINFO
    m_optionalInfoParsers["lift_height"] = [this](const QJsonObject &data)
    {
        m_liftHeight = parseAttr<int>(data, "lift_height");
    };
    m_optionalInfoParsers["Optional_Err_Msg"] = [this](const QJsonObject &data)
    {
        m_optionalErr = parseAttr<QString>(data, "Optional_Err_Msg");
    };
    // AGV_TASK
    m_agvTaskParsers["task_state"] = [this](const QJsonObject &data)
    {
        m_taskState = parseAttr<int>(data, "task_state");
    };
    m_agvTaskParsers["task_id"] = [this](const QJsonObject &data)
    {
        m_taskId = parseAttr<QString>(data, "task_id");
    };
    m_agvTaskParsers["task_start_id"] = [this](const QJsonObject &data)
    {
        m_taskStartId = parseAttr<int>(data, "task_start_id");
    };
    m_agvTaskParsers["task_start_x"] = [this](const QJsonObject &data)
    {
        m_taskStartX = parseAttr<int>(data, "task_start_x");
    };
    m_agvTaskParsers["task_start_y"] = [this](const QJsonObject &data)
    {
        m_taskStartY = parseAttr<int>(data, "task_start_y");
    };
    m_agvTaskParsers["task_end_id"] = [this](const QJsonObject &data)
    {
        m_taskEndId = parseAttr<int>(data, "task_end_id");
    };
    m_agvTaskParsers["task_end_x"] = [this](const QJsonObject &data)
    {
        m_taskEndX = parseAttr<int>(data, "task_end_x");
    };
    m_agvTaskParsers["task_end_y"] = [this](const QJsonObject &data)
    {
        m_taskEndY = parseAttr<int>(data, "task_end_y");
    };
    m_agvTaskParsers["path_start_id"] = [this](const QJsonObject &data)
    {
        m_pathStartId = parseAttr<int>(data, "path_start_id");
    };
    m_agvTaskParsers["path_start_x"] = [this](const QJsonObject &data)
    {
        m_pathStartX = parseAttr<int>(data, "path_start_x");
    };
    m_agvTaskParsers["path_start_y"] = [this](const QJsonObject &data)
    {
        m_pathStartY = parseAttr<int>(data, "path_start_y");
    };
    m_agvTaskParsers["path_end_id"] = [this](const QJsonObject &data)
    {
        m_pathEndId = parseAttr<int>(data, "path_end_id");
    };
    m_agvTaskParsers["path_end_x"] = [this](const QJsonObject &data)
    {
        m_pathEndX = parseAttr<int>(data, "path_end_x");
    };
    m_agvTaskParsers["path_end_y"] = [this](const QJsonObject &data)
    {
        m_pathEndY = parseAttr<int>(data, "path_end_y");
    };
    m_agvTaskParsers["TASK_Err_Msg"] = [this](const QJsonObject &data)
    {
        m_taskErr = parseAttr<QString>(data, "TASK_Err_Msg");
    };
}

// -- Getter --
// AGVInfo
AgvString AgvData::agvErr() const
{
    QReadLocker lockeer(&m_lock);
    return m_agvErr;
}

AgvInt AgvData::agvXin1() const
{
    QReadLocker lockeer(&m_lock);
    return m_xin1;
}

AgvInt AgvData::agvXin2() const
{
    QReadLocker lockeer(&m_lock);
    return m_xin2;
}

AgvInt AgvData::agvXin3() const
{
    QReadLocker lockeer(&m_lock);
    return m_xin3;
}

AgvInt AgvData::agvYout1() const
{
    QReadLocker lockeer(&m_lock);
    return m_yout1;
}

AgvInt AgvData::agvYout2() const
{
    QReadLocker lockeer(&m_lock);
    return m_yout2;
}

AgvInt AgvData::agvYout3() const
{
    QReadLocker lockeer(&m_lock);
    return m_yout3;
}

AgvInt AgvData::agvId() const
{
    QReadLocker lockeer(&m_lock);
    return m_agvId;
}
AgvString AgvData::agvName() const
{
    QReadLocker lockeer(&m_lock);
    return m_agvName;
}
AgvInt AgvData::battery() const
{
    QReadLocker lockeer(&m_lock);
    return m_battery;
}
AgvInt AgvData::mapId() const
{
    QReadLocker lockeer(&m_lock);
    return m_mapId;
}
AgvInt AgvData::slamX() const
{
    QReadLocker lockeer(&m_lock);
    return m_slamX;
}
AgvInt AgvData::slamY() const
{
    QReadLocker lockeer(&m_lock);
    return m_slamY;
}
AgvInt AgvData::slamAngle() const
{
    QReadLocker lockeer(&m_lock);
    return m_slamAngle;
}
AgvInt AgvData::slamCov() const
{
    QReadLocker lockeer(&m_lock);
    return m_slamCov;
}
AgvInt AgvData::vX() const
{
    QReadLocker lockeer(&m_lock);
    return m_vX;
}
AgvInt AgvData::vY() const
{
    QReadLocker lockeer(&m_lock);
    return m_vY;
}
AgvInt AgvData::vAngle() const
{
    QReadLocker lockeer(&m_lock);
    return m_vAngle;
}
AgvInt AgvData::agvMode() const
{
    QReadLocker lockeer(&m_lock);
    return m_agvMode;
}
AgvInt AgvData::agvState() const
{
    QReadLocker lockeer(&m_lock);
    return m_agvState;
}
AgvInt AgvData::light() const
{
    QReadLocker lockeer(&m_lock);
    return m_light;
}
AgvInt AgvData::frontArea() const
{
    QReadLocker lockeer(&m_lock);
    return m_frontArea;
}
AgvInt AgvData::frontLeft() const
{
    QReadLocker lockeer(&m_lock);
    return m_frontLeft;
}
AgvInt AgvData::frontRight() const
{
    QReadLocker lockeer(&m_lock);
    return m_frontRight;
}
AgvInt AgvData::backArea() const
{
    QReadLocker lockeer(&m_lock);
    return m_backArea;
}
AgvInt AgvData::backLeft() const
{
    QReadLocker lockeer(&m_lock);
    return m_backLeft;
}
AgvInt AgvData::backRight() const
{
    QReadLocker lockeer(&m_lock);
    return m_backRight;
}
AgvInt AgvData::bumpBack() const
{
    QReadLocker lockeer(&m_lock);
    return m_bumpBack;
}
AgvInt AgvData::bumpFront() const
{
    QReadLocker lockeer(&m_lock);
    return m_bumpFront;
}
AgvInt AgvData::bumpLeft() const
{
    QReadLocker lockeer(&m_lock);
    return m_bumpLeft;
}
AgvInt AgvData::bumpRight() const
{
    QReadLocker lockeer(&m_lock);
    return m_bumpRight;
}
AgvInt AgvData::eStopState() const
{
    QReadLocker lockeer(&m_lock);
    return m_estopState;
}
AgvInt AgvData::goodsState() const
{
    QReadLocker lockeer(&m_lock);
    return m_goodsState;
}
AgvInt AgvData::moveDir() const
{
    QReadLocker lockeer(&m_lock);
    return m_moveDir;
}
AgvInt AgvData::pointId() const
{
    QReadLocker lockeer(&m_lock);
    return m_pointId;
}
AgvInt AgvData::taskAct() const
{
    QReadLocker lockeer(&m_lock);
    return m_taskAct;
}
AgvInt AgvData::taskParam() const
{
    QReadLocker lockeer(&m_lock);
    return m_taskParam;
}
AgvString AgvData::taskDescription() const
{
    QReadLocker lockeer(&m_lock);
    return m_taskDescription;
}
AgvInt AgvData::runLength() const
{
    QReadLocker lockeer(&m_lock);
    return m_runLength;
}
AgvInt AgvData::runTime() const
{
    QReadLocker lockeer(&m_lock);
    return m_runTime;
}
// OptionalINFO
AgvInt AgvData::liftHeight() const
{
    QReadLocker lockeer(&m_lock);
    return m_liftHeight;
}
AgvString AgvData::optionalErr() const
{
    QReadLocker lockeer(&m_lock);
    return m_optionalErr;
}
// AGV_TASK
AgvInt AgvData::taskState() const
{
    QReadLocker lockeer(&m_lock);
    return m_taskState;
}
AgvString AgvData::taskId() const
{
    QReadLocker lockeer(&m_lock);
    return m_taskId;
}
AgvInt AgvData::taskStartId() const
{
    QReadLocker lockeer(&m_lock);
    return m_taskStartId;
}
AgvInt AgvData::taskStartX() const
{
    QReadLocker lockeer(&m_lock);
    return m_taskStartX;
}
AgvInt AgvData::taskStartY() const
{
    QReadLocker lockeer(&m_lock);
    return m_taskStartY;
}
AgvInt AgvData::taskEndId() const
{
    QReadLocker lockeer(&m_lock);
    return m_taskEndId;
}
AgvInt AgvData::taskEndX() const
{
    QReadLocker lockeer(&m_lock);
    return m_taskEndX;
}
AgvInt AgvData::taskEndY() const
{
    QReadLocker lockeer(&m_lock);
    return m_taskEndY;
}
AgvInt AgvData::pathStartId() const
{
    QReadLocker lockeer(&m_lock);
    return m_pathStartId;
}
AgvInt AgvData::pathStartX() const
{
    QReadLocker lockeer(&m_lock);
    return m_pathStartX;
}
AgvInt AgvData::pathStartY() const
{
    QReadLocker lockeer(&m_lock);
    return m_pathStartY;
}
AgvInt AgvData::pathEndId() const
{
    QReadLocker lockeer(&m_lock);
    return m_pathEndId;
}
AgvInt AgvData::pathEndX() const
{
    QReadLocker lockeer(&m_lock);
    return m_pathEndX;
}
AgvInt AgvData::pathEndY() const
{
    QReadLocker lockeer(&m_lock);
    return m_pathEndY;
}
AgvString AgvData::taskErr() const
{
    QReadLocker lockeer(&m_lock);
    return m_taskErr;
}

// 解析 JSON 结构
bool AgvData::tryParseAgvJson(const QString &jsonStr, QJsonObject &resultObj)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError)
    {
        qWarning() << "AgvData JSON 解析错误:" << parseError.errorString();
        return false;
    }

    if (!doc.isObject())
    {
        qWarning() << "AgvData 数据格式错误: 不是 JSON 对象";
        return false;
    }

    resultObj = doc.object();
    return true;
}

// parseMsg: 入口函数
void AgvData::parseMsg(const QString &msg)
{
    QJsonObject root;

    if (!tryParseAgvJson(msg, root))
    {
        qDebug() << "parseMsg 发生错误，非法的 json 结构";
        return; // 校验失败
    }

    // 提取 Event 类型
    if (!root.value("Event").isString())
    {
        qWarning() << "AgvData 缺少 Event 字段 或者 Event 字段不是 String 类型";
        return;
    }
    QString event = root.value("Event").toString();

    // 提取 Body 数据实体
    // 注意：Body 可能是 Object，也可能是 Array，这里假设你的业务数据主要是 Object
    if (!root.value("Body").isObject())
    {
        qWarning() << "AgvData 缺少 Body 字段 或者 Body 字段不是 JSON 对象";
        return;
    }
    QJsonObject body = root.value("Body").toObject();

    // 根据 Event 分发处理
    if (event == "AGV_STATE")
    {
        parseAgvState(body);
    }
    else if (event == "AGV_TASK")
    {
        handleAgvTask(body);
    }
    else
    {
        qDebug() << "忽略未知的事件类型:" << event;
    }
}

// 解析 AgvState，包含 AGVInfo、OptionalINFO
void AgvData::parseAgvState(const QJsonObject &data)
{
    // 解析 AGVInfo
    // 校验字段是否存在
    if (!data.value("AGVInfo").isObject())
    {
        qWarning() << "AGV_STATE 错误: 缺少 AGVInfo 字段或者 AGVInfo 不是 JSON 对象";
        return;
    }

    // 安全转换为 QJsonObject
    QJsonObject agvInfo = data.value("AGVInfo").toObject();
    qDebug() << "解析 AGVInfo:" << agvInfo;
    handleAgvInfo(agvInfo);

    // 解析 OptionalINFO
    // 校验字段是否存在
    if (!data.value("OptionalINFO").isObject())
    {
        qWarning() << "AGV_STATE 错误: 缺少 OptionalINFO 字段或者 OptionalINFO 不是 JSON 对象";
        return;
    }

    // 安全转换为 QJsonObject
    QJsonObject optionalInfo = data.value("OptionalINFO").toObject();
    qDebug() << "解析 OptionalINFO:" << optionalInfo;
    handleOptionalInfo(optionalInfo);
}

// 处理 AGVInfo
void AgvData::handleAgvInfo(const QJsonObject &data)
{
    // 加锁
    QReadLocker locker(&m_lock);

    // 遍历 JSON 中的所有 key
    for (auto it = data.constBegin(); it != data.constEnd(); ++it)
    {
        QString key = it.key();

        // 如果这个 key 在我们的映射表中注册过，就执行对应的解析函数
        if (m_agvInfoParsers.contains(key))
        {
            m_agvInfoParsers[key](data);
        }
    }

    qDebug() << "Xin3.value: " << m_xin3.value << ", Yout3.value: " << m_yout3.value;
}

// 处理 OptionalINFO
void AgvData::handleOptionalInfo(const QJsonObject &data)
{
    // 加锁
    QReadLocker locker(&m_lock);

    // 遍历 JSON 中的所有 key
    for (auto it = data.constBegin(); it != data.constEnd(); ++it)
    {
        QString key = it.key();

        if (m_optionalInfoParsers.contains(key))
        {
            m_optionalInfoParsers[key](data);
        }
    }

    qDebug() << "lift_height.value: " << m_liftHeight.value << ", optionalErr.value: " << m_optionalErr.value;
}

// 处理 AGV_TASK
void AgvData::handleAgvTask(const QJsonObject &data)
{
    // 加锁
    QReadLocker locker(&m_lock);

    // 遍历 JSON 中的所有 key
    for (auto it = data.constBegin(); it != data.constEnd(); ++it)
    {
        QString key = it.key();

        if (m_agvTaskParsers.contains(key))
        {
            m_agvTaskParsers[key](data);
        }
    }

    qDebug() << "task_start_id.value: " << m_taskStartId.value << ", taskErr.value: " << m_taskErr.value;
}
