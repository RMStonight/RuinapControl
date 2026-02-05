#include "monitor/MapDataManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

MapDataManager::MapDataManager(QObject *parent) : QObject(parent)
{
}

bool MapDataManager::parseMapJson(const QString &path,
                                  QVector<MapPointData> &outPoints,
                                  QVector<MapPathData> &outPaths)
{
    // 1. 读取文件
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        logger->log(QStringLiteral("MapDataManager"), spdlog::level::err, QStringLiteral("无法打开JSON文件: %1").arg(path));
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    // 2. 解析 JSON 文档
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        logger->log(QStringLiteral("MapDataManager"), spdlog::level::err, QStringLiteral("JSON 解析错误: %1").arg(parseError.errorString()));
        return false;
    }

    if (!doc.isObject())
        return false;
    QJsonObject jsonObj = doc.object();

    // 校验必要的 "point" 数组字段
    if (!jsonObj.contains("point") || !jsonObj["point"].isArray())
    {
        logger->log(QStringLiteral("MapDataManager"), spdlog::level::err, QStringLiteral("JSON 中未找到 'point' 数组"));
        return false;
    }

    QJsonArray pointsArray = jsonObj["point"].toArray();

    // 3. 第一轮遍历：建立 ID 到原始 JSON 对象的映射索引
    m_pointMap.clear();
    for (int i = 0; i < pointsArray.size(); ++i)
    {
        QJsonObject pObj = pointsArray[i].toObject();
        int id = pObj.value("id").toInt();
        m_pointMap.insert(id, pObj);
    }

    // 清空输出容器
    outPoints.clear();
    outPaths.clear();

    // 4. 第二轮遍历：构造显示用的结构化数据
    for (int i = 0; i < pointsArray.size(); ++i)
    {
        QJsonObject pointObj = pointsArray[i].toObject();
        int startId = pointObj.value("id").toInt();

        // 原始数据为 mm，转换为渲染使用的 m
        QPointF startPos(pointObj.value("x").toDouble() / 1000.0,
                         pointObj.value("y").toDouble() / 1000.0);

        // --- 处理点位渲染数据 ---
        MapPointData pData;
        pData.pos = startPos;
        pData.id = QString::number(startId);

        // 业务逻辑颜色分配
        bool isCharge = pointObj.value("charge").toBool();
        bool isAct = pointObj.value("loading").toBool() || pointObj.value("unloading").toBool();

        if (isCharge)
        {
            pData.color = QColor(0, 120, 215, 180); // 充电点：蓝色
        }
        else if (isAct)
        {
            pData.color = QColor(215, 120, 0, 180); // 动作点：棕色
        }
        else
        {
            pData.color = QColor(255, 0, 0, 180); // 默认：红色
        }
        outPoints.append(pData);

        // --- 处理路径渲染数据 (targets 数组) ---
        if (pointObj.contains("targets") && pointObj["targets"].isArray())
        {
            QJsonArray targetsArray = pointObj["targets"].toArray();
            for (int j = 0; j < targetsArray.size(); ++j)
            {
                QJsonObject tObj = targetsArray[j].toObject();
                int endId = tObj.value("id").toInt();

                // 只有当目标点 ID 在索引中存在时才建立路径
                if (m_pointMap.contains(endId))
                {
                    QJsonObject targetPoint = m_pointMap.value(endId);
                    MapPathData pathData;
                    pathData.start = startPos;
                    pathData.end = QPointF(targetPoint.value("x").toDouble() / 1000.0,
                                           targetPoint.value("y").toDouble() / 1000.0);

                    pathData.type = tObj.value("type").toInt();

                    // 解析控制点并转换单位 (mm -> m)
                    QJsonObject c1 = tObj.value("ctl_1").toObject();
                    pathData.ctl1 = QPointF(c1.value("x").toDouble() / 1000.0,
                                            c1.value("y").toDouble() / 1000.0);

                    QJsonObject c2 = tObj.value("ctl_2").toObject();
                    pathData.ctl2 = QPointF(c2.value("x").toDouble() / 1000.0,
                                            c2.value("y").toDouble() / 1000.0);

                    outPaths.append(pathData);
                }
            }
        }
    }

    return true;
}

QJsonObject MapDataManager::getPointInfo(int id) const
{
    return m_pointMap.value(id, QJsonObject());
}

const QMap<int, QJsonObject> &MapDataManager::getPointMap() const
{
    return m_pointMap;
}