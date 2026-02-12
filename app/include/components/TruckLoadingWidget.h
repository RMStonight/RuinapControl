#ifndef TRUCKLOADINGWIDGET_H
#define TRUCKLOADINGWIDGET_H

#include "BaseDisplayWidget.h"
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>
#include "LogManager.h"
#include "utils/TruckWsClient.h"
#include <QTimer>

class TruckLoadingWidget : public BaseDisplayWidget
{
    Q_OBJECT
public:
    explicit TruckLoadingWidget(QWidget *parent = nullptr);

signals:
    /**
     * @brief 通知外部发送WS请求
     */
    void requestTruckSize();

public slots:
    /**
     * @brief 外部解析完WS数据后调用此方法更新UI
     * @param timestamp 时间戳字符串
     * @param width 车厢宽度
     * @param depth 车厢深度
     */
    void updateTruckData(const QString &timestamp, int width, int depth);

    void handleGetSizeBtn();
    void handleGetSizeTimeout();

private:
    // 日志管理器
    LogManager *logger = &LogManager::instance();

    void initUi();

    // UI 控件
    QPushButton *m_getSizeBtn;
    QLabel *m_timeLabel;
    QLabel *m_widthLabel;
    QLabel *m_depthLabel;

    // 内部样式常量
    const QString LABEL_STYLE = "font-weight: bold; margin-left: 15px;";

    const QString btn_unlock = "QPushButton {"
                               "  background-color: #2196F3;"
                               "  color: white;"
                               "  border-radius: 4px;"
                               "  font-weight: bold;"
                               "}"
                               "QPushButton:hover { background-color: #1E88E5; }"
                               "QPushButton:pressed { background-color: #1976D2; }";

    const QString btn_lock = "QPushButton {"
                             "  background-color: #ccc;"
                             "  color: white;"
                             "  border-radius: 4px;"
                             "  font-weight: bold;"
                             "}";

    QTimer *m_timeoutTimer;       // 超时定时器
    bool m_isWaitingData = false; // 是否正在等待数据标志位
};

#endif // TRUCKLOADINGWIDGET_H