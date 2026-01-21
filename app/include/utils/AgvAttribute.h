#ifndef AGVATTRIBUTE_H
#define AGVATTRIBUTE_H

#include <QString>
#include <QMetaType> // 用于信号槽传递自定义类型

// 定义一个模板类，T 可以是 double, int, QString 等
template <typename T>
struct AgvAttribute {
    T value;        // 实际的值
    QString color;  // 服务端指定的颜色

    // 默认构造
    AgvAttribute() : value(T()), color("#000000") {}

    // 构造函数
    AgvAttribute(T v, const QString &c) : value(v), color(c) {}

    // 重载 != 运算符，用于判断数据是否发生变化，减少无效 UI 刷新
    bool operator!=(const AgvAttribute<T> &other) const {
        // 对于 double 最好用 qFuzzyCompare，这里简化处理
        return value != other.value || color != other.color;
    }
};

// 必须注册，否则不能在信号槽中直接作为参数传递
// 注意：模板类的注册比较特殊，通常我们在使用的地方声明别名
#endif // AGVATTRIBUTE_H