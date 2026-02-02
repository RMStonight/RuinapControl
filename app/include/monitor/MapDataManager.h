#ifndef MAPDATAMANAGER_H
#define MAPDATAMANAGER_H

#include <QObject>
#include <QPointF>
#include <QJsonObject>
#include <QMap>
#include <QVector>
#include <QString>
#include <QColor>
#include "layers/PointPathLayer.h" // 必须包含以使用 MapPointData 和 MapPathData 结构体

class MapDataManager : public QObject {
    Q_OBJECT
public:
    explicit MapDataManager(QObject *parent = nullptr);

    // 解析 JSON 文件并提取用于渲染的点位和路径数据
    bool parseMapJson(const QString &path, 
                      QVector<MapPointData> &outPoints, 
                      QVector<MapPathData> &outPaths);

    // 根据点位 ID 获取原始 JSON 对象信息（用于点击后的详细业务逻辑）
    QJsonObject getPointInfo(int id) const;

    // 获取当前缓存的所有点位映射
    const QMap<int, QJsonObject>& getPointMap() const;

private:
    // 内部缓存，Key 为点位 ID
    QMap<int, QJsonObject> m_pointMap;
};

#endif // MAPDATAMANAGER_H