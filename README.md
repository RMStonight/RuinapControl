# This is a README

## 20260121 V1.1.3

* RosBridgeClient 新增重连机制
* 新增 WebsocketClient 通用 websocket 客户端模块
* 新增 CommunicationWsClient，用于调用 WebsocketClient 实现定制化的 wsClient 业务处理，定时发送轮询请求，并将接收到的消息 emit
* 新增 AgvData，单例模式，作为 Agv 相关参数的全局变量。包含 AgvInfo、optionalINFO、AGV_TASk。并实现解析 CommunicationWsClient 接收到的消息
* 在 TopHeaderWidget UI 区域中，通过定时器定时更新，根据最新的 AgvData 对应内容来更新。可以设置为较低频率

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