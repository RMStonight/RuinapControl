#ifndef OPTIONALINFOWIDGET_H
#define OPTIONALINFOWIDGET_H

#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QWidget>
#include <QTimer>
#include <QMap>

class OptionalInfoWidget : public QScrollArea
{
    Q_OBJECT
public:
    explicit OptionalInfoWidget(QWidget *parent = nullptr);
    ~OptionalInfoWidget();

    void setInfoRow(const QString &key, const QString &value, const QString &color = "#000000");

private slots:
    void updateUi();

private:
    QWidget *m_contentWidget;     
    QVBoxLayout *m_contentLayout; 

    QTimer *m_updateTimer;
    const int UPDATE_INTERVAL_MS = 100;

    // [新增] 注册表：Key -> 显示数值的 Label 指针
    // 用于快速查找某一行是否存在，从而决定是 update 还是 new
    QMap<QString, QLabel*> m_rowMap;
};

#endif // OPTIONALINFOWIDGET_H