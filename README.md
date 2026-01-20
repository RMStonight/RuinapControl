# This is a README

## 20260120 V1.1.2

* 新增了 BottomInfoBar，用于在部分 tab 标签页底部显示信息栏
* 删除了 RosBridgeClient 中对 map 的订阅与交互
* 现在直接通过 map 的指定文件夹来加载 png 图片
* ConfigManager 中添加了线程安全操作，autmic 以及 QReadWriteLock

## 20260119 V1.1.1

* 删除了原先的 RosDataWorker 线程，将 RosBridgeClient 改为子线程运行，并将逻辑处理也放在该线程中

## 20260116 V1.1.0

* 新增 Version.h 来设置版本号
* 优化 TopHeaderWidget 组件
* 新增 light.svg 图标