#ifndef ROSDATAWORKER_H
#define ROSDATAWORKER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QImage>
#include <QJsonArray>
#include <QVector>
#include <QPointF>
#include <cmath>
#include <QDebug>
#include <QCborValue>
#include <QCborMap>
#include <QCborArray>

class RosDataWorker : public QObject
{
    Q_OBJECT
public:
    explicit RosDataWorker(QObject *parent = nullptr) : QObject(parent) {}

signals:
    void mapParsed(QImage mapImg, double originX, double originY, double resolution);
    void scanParsed(QVector<QPointF> points);

public slots:
    void processRawMessage(QString rawMsg)
    {
        QJsonDocument doc = QJsonDocument::fromJson(rawMsg.toUtf8());
        if (!doc.isNull())
        { /* 可选: 保留 JSON 处理 */
        }
    }

    void processCborMessage(QByteArray rawData)
    {
        QCborParserError error;
        QCborValue val = QCborValue::fromCbor(rawData, &error);

        if (error.error != QCborError::NoError)
        {
            qDebug() << "Worker: CBOR Decode Error:" << error.errorString();
            return;
        }

        if (val.isMap())
        {
            QCborMap map = val.toMap();
            QString op = map[QStringLiteral("op")].toString();
            if (op != "publish")
                return;

            QString topic = map[QStringLiteral("topic")].toString();
            QCborValue msg = map[QStringLiteral("msg")];

            if (topic == "/map")
            {
                parseMapCbor(msg);
            }
            else if (topic == "/scan")
            {
                parseScanCbor(msg);
            }
        }
    }

private:
    // [辅助函数] 剥离 Tag 获取真实的二进制数据
    QByteArray extractByteArray(const QCborValue &val)
    {
        if (val.isByteArray())
        {
            return val.toByteArray();
        }
        // [关键修复] 如果是 Tag 类型 (rosbridge 发送 Typed Array 时会加 Tag)
        if (val.isTag())
        {
            // 获取被 Tag 标记的内容 (Tagged Value)
            QCborValue taggedVal = val.taggedValue();
            if (taggedVal.isByteArray())
            {
                return taggedVal.toByteArray();
            }
        }
        return QByteArray();
    }

    // --- 地图解析 (修复了字节对齐导致的形变问题) ---
    void parseMapCbor(const QCborValue &msgVal)
    {
        QCborMap msg = msgVal.toMap();
        QCborMap info = msg[QStringLiteral("info")].toMap();

        int width = info[QStringLiteral("width")].toInteger();
        int height = info[QStringLiteral("height")].toInteger();
        double resolution = info[QStringLiteral("resolution")].toDouble();

        QCborMap origin = info[QStringLiteral("origin")].toMap()[QStringLiteral("position")].toMap();
        double originX = origin[QStringLiteral("x")].toDouble();
        double originY = origin[QStringLiteral("y")].toDouble();

        // 1. 获取数据
        QByteArray mapData = extractByteArray(msg[QStringLiteral("data")]);

        // 兼容性处理
        if (mapData.isEmpty() && !msg[QStringLiteral("data")].isByteArray() && !msg[QStringLiteral("data")].isTag())
        {
            if (msg[QStringLiteral("data")].isArray())
            {
                QCborArray arr = msg[QStringLiteral("data")].toArray();
                mapData.resize(arr.size());
                char *ptr = mapData.data();
                for (auto v : arr)
                    *ptr++ = static_cast<char>(v.toInteger());
            }
        }

        // 检查数据长度
        int totalPixels = width * height;
        if (mapData.size() < totalPixels)
            return;

        // 2. 构建 QImage
        // 使用 Format_RGB888，Qt 会自动处理每行的 4字节对齐
        QImage image(width, height, QImage::Format_RGB888);

        // 获取 ROS 原始数据的指针
        const char *rawDataPtr = mapData.constData();

        // 3. 逐行填充 (关键修复)
        for (int y = 0; y < height; ++y)
        {
            // 直接使用 y，不进行翻转。
            // 这样生成的 QImage 看起来是“倒”的，但配合 MonitorWidget 的 scale(1, -1) 显示就是正的。
            const char *rosLinePtr = rawDataPtr + (y * width);

            // [关键] 获取 QImage 这一行的内存首地址 (scanLine 会自动跳过 Padding)
            uchar *qtLinePtr = image.scanLine(y);

            for (int x = 0; x < width; ++x)
            {
                char val = rosLinePtr[x];

                // 指针移动：RGB888 占 3 字节
                int byteIdx = x * 3;

                if (val == -1)
                { // Unknown (灰色)
                    qtLinePtr[byteIdx] = 128;
                    qtLinePtr[byteIdx + 1] = 128;
                    qtLinePtr[byteIdx + 2] = 128;
                }
                else if (val == 0)
                { // Free (白色)
                    qtLinePtr[byteIdx] = 255;
                    qtLinePtr[byteIdx + 1] = 255;
                    qtLinePtr[byteIdx + 2] = 255;
                }
                else
                { // Occupied (黑色)
                    qtLinePtr[byteIdx] = 0;
                    qtLinePtr[byteIdx + 1] = 0;
                    qtLinePtr[byteIdx + 2] = 0;
                }
            }
        }

        // 发送结果
        // image 已经是正的了，所以 MonitorWidget 里的绘制逻辑可能需要微调
        // 如果之前 MonitorWidget 有 scale(1, -1)，现在可能需要去掉，或者反过来
        emit mapParsed(image, originX, originY, resolution);
    }

    void parseScanCbor(const QCborValue &msgVal)
    {
        QCborMap msg = msgVal.toMap();
        double angle_min = msg[QStringLiteral("angle_min")].toDouble();
        double angle_increment = msg[QStringLiteral("angle_increment")].toDouble();
        double range_min = msg[QStringLiteral("range_min")].toDouble();
        double range_max = msg[QStringLiteral("range_max")].toDouble();

        QCborValue rangesVal = msg[QStringLiteral("ranges")];
        QVector<QPointF> points;

        // 1. 尝试提取二进制数据 (Tag 或者 直接 ByteArray)
        QByteArray data = extractByteArray(rangesVal);

        if (!data.isEmpty())
        {
            // 二进制 Float32 数组处理
            int count = data.size() / 4;
            points.reserve(count);
            const float *rawRanges = reinterpret_cast<const float *>(data.constData());

            for (int i = 0; i < count; ++i)
            {
                float r = rawRanges[i];
                if (std::isfinite(r) && r > range_min && r < range_max)
                {
                    double angle = angle_min + i * angle_increment;
                    points.append(QPointF(r * cos(angle), r * sin(angle)));
                }
            }
        }
        else if (rangesVal.isArray())
        {
            // 普通数组处理
            QCborArray ranges = rangesVal.toArray();
            points.reserve(ranges.size());
            for (int i = 0; i < ranges.size(); ++i)
            {
                double r = ranges[i].toDouble();
                if (r > range_min && r < range_max)
                {
                    double angle = angle_min + i * angle_increment;
                    points.append(QPointF(r * cos(angle), r * sin(angle)));
                }
            }
        }
        else
        {
            qDebug() << "Worker: Scan ranges format unknown! Type:" << rangesVal.type();
        }

        emit scanParsed(points);
    }
};

#endif // ROSDATAWORKER_H