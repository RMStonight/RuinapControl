cmake_minimum_required(VERSION 3.10)
project(RuinapControl)

# 基础设置
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

# 查找 Qt 库
find_package(Qt5 REQUIRED COMPONENTS Widgets Svg WebSockets SerialPort)

# 定义源文件列表 (包含新添加的组件文件)
set(SOURCES
    src/main.cpp
    src/utils/ConfigManager.cpp
    src/utils/RosBridgeClient.cpp
    src/utils/WebsocketClient.cpp
    src/utils/CommunicationWsClient.cpp
    src/utils/NetworkCheckThread.cpp
    src/utils/AgvData.cpp
    src/monitor/MapDataManager.cpp
    src/monitor/MonitorInteractionHandler.cpp
    src/monitor/RelocationController.cpp
    src/MainWindow.cpp
    src/components/TopHeaderWidget.cpp
    src/components/MainContentWidget.cpp
    src/components/SystemSettingsWidget.cpp
    src/components/VehicleInfoWidget.cpp
    src/components/MonitorWidget.cpp
    src/components/BottomInfoBar.cpp
    src/components/OptionalInfoWidget.cpp
    src/components/IoWidget.cpp
    src/components/ManualControlWidget.cpp
    src/components/SerialDebugWidget.cpp
    src/components/LogDisplayWidget.cpp

    include/utils/ConfigManager.h
    include/utils/RosBridgeClient.h
    include/utils/WebsocketClient.h
    include/utils/CommunicationWsClient.h
    include/utils/NetworkCheckThread.h
    include/utils/AgvData.h
    include/utils/PermissionManager.h
    include/utils/GlobalEventFilter.h
    include/utils/LogManager.h
    include/monitor/MapDataManager.h
    include/monitor/MonitorInteractionHandler.h
    include/monitor/RelocationController.h
    include/MainWindow.h
    include/components/TopHeaderWidget.h
    include/components/MainContentWidget.h
    include/components/SystemSettingsWidget.h
    include/components/VehicleInfoWidget.h
    include/components/MonitorWidget.h
    include/components/BottomInfoBar.h
    include/components/OptionalInfoWidget.h
    include/components/IoWidget.h
    include/components/BaseDisplayWidget.h
    include/components/ManualControlWidget.h
    include/components/SerialDebugWidget.h
    include/components/LogDisplayWidget.h
    include/layers/BaseLayer.h
    include/layers/GridLayer.h
    include/layers/MapLayer.h
    include/layers/AgvDrawer.h
    include/layers/AgvLayer.h
    include/layers/PointPathLayer.h
    include/layers/RelocationLayer.h
    include/layers/FixedRelocationLayer.h

    resources/app.qrc
)

# ========================================================
# 必须先创建可执行文件 (Target)，后面的 target_... 命令才能生效！
# ========================================================
add_executable(RuinapControl ${SOURCES})

# ========================================================
# 在创建之后，再设置包含路径和链接库
# ========================================================

# 添加头文件搜索路径
# 这里把 include 和 include/components 都加进去
# 这样你在代码里写 #include "TopHeaderWidget.h" 或 #include "components/TopHeaderWidget.h" 都可以
target_include_directories(RuinapControl PRIVATE 
    include
    include/components 
    include/utils
)

# 链接 Qt 库
target_link_libraries(RuinapControl 
    Qt5::Widgets
    Qt5::Svg
    Qt5::WebSockets
    Qt5::SerialPort
)