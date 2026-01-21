cmake_minimum_required(VERSION 3.10)
project(RuinapControl)

# 基础设置
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

# 查找 Qt 库
find_package(Qt5 REQUIRED COMPONENTS Widgets Svg WebSockets)

# 定义源文件列表 (包含新添加的组件文件)
set(SOURCES
    src/main.cpp
    src/utils/ConfigManager.cpp
    src/utils/RosBridgeClient.cpp
    src/utils/WebsocketClient.cpp
    src/utils/CommunicationWsClient.cpp
    src/utils/AgvData.cpp
    src/MainWindow.cpp
    src/components/TopHeaderWidget.cpp
    src/components/MainContentWidget.cpp
    src/components/SystemSettingsWidget.cpp
    src/components/MonitorWidget.cpp
    src/components/BottomInfoBar.cpp

    include/utils/ConfigManager.h
    include/utils/RosBridgeClient.h
    include/utils/WebsocketClient.h
    include/utils/CommunicationWsClient.h
    include/utils/AgvData.h
    include/MainWindow.h
    include/components/TopHeaderWidget.h
    include/components/MainContentWidget.h
    include/components/SystemSettingsWidget.h
    include/components/MonitorWidget.h
    include/components/BottomInfoBar.h

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
)