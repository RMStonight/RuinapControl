#ifndef BOTTOMINFOBAR_H
#define BOTTOMINFOBAR_H

#include <QWidget>
#include <QMap>
#include <QTimer>
#include "LogManager.h"

class QGridLayout;
class QLabel;

class BottomInfoBar : public QWidget
{
    Q_OBJECT
public:
    explicit BottomInfoBar(QWidget *parent = nullptr);
    ~BottomInfoBar();

    // 更新接口
    void updateValue(const QString &key, const QString &value);
    void updateAllValues(const QMap<QString, QString> &data);

private slots:
    void updateUi();

signals:
    void mapIdChanged(int mapId);

private:
    // 日志管理器
    LogManager *logger = &LogManager::instance();

    const QString lblKeyStyle = "color: #666666; font-size: 12px;";
    const QString lblValueStyleGreen = "color: #007055; font-size: 13px;";
    const QString lblValueStyleRed = "color: red; font-weight: bold; font-size: 13px;";
    void initLayout();

    // 辅助函数：注意这里的参数类型必须和 .cpp 完全一致
    void addStatusItem(QGridLayout *layout, const QString &key, int row, int col, int colSpan = 1);

    // 存储所有的数值 Label
    QMap<QString, QLabel *> m_valueLabels;

    // 特别记录 mapId
    int m_mapId = -1;

    // 定时器更新 UI
    QTimer *m_updateTimer;
    const int UPDATE_INTERVAL_MS = 100;

    // 解析 label 需要显示的内容
    const QString handleMoveDir(int moveDir);
    const QString handleGoodsState(int goodsState);
};

#endif // BOTTOMINFOBAR_H