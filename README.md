# This is a README

## 20260127 V1.1.6

* 不再处理 OptionalInfoErr，不存在该字段
* 切换至 SystemSettingsWidget 时加载一次配置文件
* 不再通过 ros 订阅 topic 的 map_name 来切换地图，而是使用 AGV_STATE 中的 map_id 字段
* 提取出 BaseDisplayWidget 基类，实现统一的左右布局，左侧 3/4 为页面主内容，右侧 1/4 为 OptionalInfoWidget
* 将 VehicleInfoWidget、MonitorWidget、IoWidget 页面全部修改为使用 BaseDisplayWidget 基类

## 20260123 V1.1.5

* 系统配置新增 mapResolution 参数
* RosBridgeClient 中新增对 map_name、agv_state 的订阅与处理
* MonitorWidget 新增地图切换逻辑，并且绘制了实时的 agv 以及 点云
* AgvData 中新增备份 optionalInfo，用于可选信息的显示
* 新增 OptionalInfoWidget 组件，当前添加至 车辆信息的右侧
* 新增 IoWidget 组件，用于显示 24 个 Input 信号和 24 个 Output 信号

## 20260122 V1.1.4

* 系统配置新增了 vehicleType 和 configFolder 两个参数
* BottomInfoBar 组件中添加定时器，用于定期更新内容
* 新增 VehicleInfoWidget 组件，用于显示 车辆信息 页面，包括车体模型图片，防撞条以及避障区域，有无货物 的显示
* 有无货物的显示，根据系统的 vehicleType 判断是 左右货 还是 一个货
* TopHeaderWidget UI 区域中，右上角将 备用字段 利用，显示网络状态
* 新增 NetworkCheckThread 子线程，用于检测网络连接
* 检测网络连接时，优先检测 eth_ip.json 中的成员，如果有 ping 不通的，直接显示 xxx 离线
* 检测网络连接时，如果 eth_ip.json 中的成员都正常 ping 通，那么显示与服务器间的延迟，服务器 ip 从系统配置中读取
* 检测网络连接时，如果 eth_ip.json 中的成员都正常 ping 通，但是服务器 ping 不通，那么显示 服务器离线

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