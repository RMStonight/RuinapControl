#ifndef BOTTOMINFOBAR_H
#define BOTTOMINFOBAR_H

#include <QWidget>
#include <QMap>

class QGridLayout; 
class QLabel;

class BottomInfoBar : public QWidget
{
    Q_OBJECT
public:
    explicit BottomInfoBar(QWidget *parent = nullptr);

    // 更新接口
    void updateValue(const QString &key, const QString &value);
    void updateAllValues(const QMap<QString, QString> &data);

private:
    void initLayout();

    // 辅助函数：注意这里的参数类型必须和 .cpp 完全一致
    void addStatusItem(QGridLayout *layout, const QString &key, int row, int col, int colSpan = 1);

    // 存储所有的数值 Label
    QMap<QString, QLabel*> m_valueLabels;
};

#endif // BOTTOMINFOBAR_H