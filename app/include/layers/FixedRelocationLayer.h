#ifndef FIXEDRELOCATIONLAYER_H
#define FIXEDRELOCATIONLAYER_H

#include "BaseLayer.h"
#include <QtMath>
#include <QVector>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "utils/ConfigManager.h"
#include "AgvDrawer.h"
#include <QDir>
#include "LogManager.h"

#define INITIAL_POINTS_JSON "initial_points.json"

// 结构体定义保持不变
struct FixedPose
{
    double x;     // m
    double y;     // m
    double angle; // rad
};

// 全局变量或静态变量，用于存储 JSON 中的 initial_points
// 建议在实际项目中管理好该变量的生命周期
static QJsonArray m_initialPoints;

class FixedRelocationLayer : public BaseLayer
{
public:
    FixedRelocationLayer()
    {
        setVisible(false);
        initFixedPoses(); // 启动时加载 JSON 配置文件
    }

    void draw(QPainter *painter) override
    {
        if (m_poses.isEmpty())
            return;

        int vehicleType = ConfigManager::instance()->vehicleType();
        QColor themeColor(40, 167, 69, 150); // 半透明绿色

        for (const auto &pose : m_poses)
        {
            painter->save();
            // 世界坐标 -> 绘图坐标
            painter->translate(pose.x, -pose.y);
            painter->rotate(-qRadiansToDegrees(pose.angle));
            painter->scale(1, -1);

            AgvDrawer::draw(painter, vehicleType, themeColor, 1.0);
            painter->restore();
        }
    }

    /**
     * 需求 2 & 3 & 4: 根据 mapId 更新当前显示的固定点位
     */
    void update(int mapId)
    {
        // 1. 清空当前画布的点位数据
        m_poses.clear();

        // 2. 遍历全局缓存的 JSON 对象数组
        for (const QJsonValue &value : m_initialPoints)
        {
            QJsonObject obj = value.toObject();
            if (obj.contains("map_id") && obj["map_id"].toInt() == mapId)
            {
                // 3. 找到匹配的 mapId，读取其 points 数组
                if (obj.contains("points") && obj["points"].isArray())
                {
                    QJsonArray pointsArr = obj["points"].toArray();
                    for (const QJsonValue &pVal : pointsArr)
                    {
                        QJsonObject pObj = pVal.toObject();
                        FixedPose pose;
                        // 转换逻辑：mm -> m, 度*100 -> rad
                        pose.x = pObj["x"].toDouble() / 1000.0;
                        pose.y = pObj["y"].toDouble() / 1000.0;
                        pose.angle = qDegreesToRadians(pObj["w"].toDouble() / 100.0);
                        m_poses.append(pose);
                    }
                }
                break; // 找到后退出循环
            }
        }

        // 提示：通常此处需要触发界面刷新，例如 update();
    }

    int checkHit(const QPointF &worldPos)
    {
        if (!isVisible())
            return -1;
        double threshold = 1.0;
        for (int i = 0; i < m_poses.size(); ++i)
        {
            double dx = worldPos.x() - m_poses[i].x;
            double dy = worldPos.y() - m_poses[i].y;
            if (std::sqrt(dx * dx + dy * dy) < threshold)
                return i;
        }
        return -1;
    }

    FixedPose getPose(int index) const
    {
        if (index >= 0 && index < m_poses.size())
            return m_poses[index];
        return {0, 0, 0};
    }

private:
    /**
     * 需求 1: 载入指定路径的 json 文件
     */
    void initFixedPoses()
    {
        // 假设配置文件路径，实际可根据需要修改
        QString configFolder = ConfigManager::instance()->configFolder();
        QString filePath = QDir(configFolder).filePath(INITIAL_POINTS_JSON);
        QFile file(filePath);

        if (!file.open(QIODevice::ReadOnly))
        {
            logger->log("FixedRelocationLayer", spdlog::level::err, QString("Failed to open JSON file: %1").arg(filePath));
            return;
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull() || !doc.isObject())
        {
            logger->log("FixedRelocationLayer", spdlog::level::err, QString("Invalid JSON format."));
            return;
        }

        QJsonObject root = doc.object();
        if (root.contains("initial_points") && root["initial_points"].isArray())
        {
            // 保存到全局变量中
            m_initialPoints = root["initial_points"].toArray();
        }
    }

private:
    // 日志管理器
    LogManager *logger = &LogManager::instance();

    QJsonArray m_initialPoints;
    QVector<FixedPose> m_poses; // 当前地图正在渲染的点位
};

#endif