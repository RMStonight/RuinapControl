# Qt 开发备忘

## XQuartz

```bash
xhost +localhost
```

编译指令

```bash
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build -S .
cmake --build build -j
./build/RuinapControl
```

容器内部访问宿主机端口

```bash
"ws://host.docker.internal:9001"
```

## 工控机上打包

### 下载 linuxdeployqt

```bash
# 下载 (以 x86_64 架构为例)
wget https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage

# 重命名为简单的命令
mv linuxdeployqt-continuous-x86_64.AppImage linuxdeployqt

# 赋予执行权限
chmod +x linuxdeployqt

# (可选) 移动到系统目录以便全局调用，或者就在当前目录使用
sudo mv linuxdeployqt /usr/local/bin/
```

### 下载 appimagetool

```bash
# 下载官方的 AppImage 打包工具
wget https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage

# 赋予执行权限
chmod +x appimagetool-x86_64.AppImage
```

### 脚本打包

* 工程目录下(即 CMakeLists.txt 目录下) 需要 linuxdeployqt、appimagetool、logo.png
* 工程目录下新增如下的 build_package.sh
* 将其设置为可执行文件后，在当前目录下运行即可，等待编译并打包成单独的可执行文件，例如 RuinapControl-x86_64.AppImage

```bash
#!/bin/bash

# ================= 配置区域 =================
APP_NAME="RuinapControl"
BUILD_DIR="build"
APP_DIR="AppDir"
ICON_SOURCE="logo.png"
ICON_NAME="ruinap"
LOG_FILE="build_output.log" # 日志文件路径

# 工具路径
LINUXDEPLOY="linuxdeployqt"
APPIMAGETOOL="./appimagetool-x86_64.AppImage"
# ===========================================

set -e # 遇到错误立即停止

# === ✨ 核心功能：带计时的命令执行器 ===
run_with_timer() {
    local message="$1"
    shift
    local command="$@"
    
    # 隐藏光标
    tput civis
    
    # 记录开始时间
    local start_ts=$(date +%s)
    
    # 在后台执行命令，将标准输出和错误输出都重定向到日志文件
    eval "$command" > "$LOG_FILE" 2>&1 &
    local pid=$!
    
    # 循环显示计时，直到进程结束
    while kill -0 "$pid" 2>/dev/null; do
        local now_ts=$(date +%s)
        local elapsed=$((now_ts - start_ts))
        local min=$((elapsed / 60))
        local sec=$((elapsed % 60))
        
        # \r 回车不换行，实现原地刷新
        printf "\r⏳ [ %02d:%02d ] %s" "$min" "$sec" "$message"
        sleep 0.5
    done
    
    # 等待进程完全退出并获取退出码
    wait "$pid"
    local exit_code=$?
    
    # 计算最终耗时
    local end_ts=$(date +%s)
    local total_elapsed=$((end_ts - start_ts))
    local total_min=$((total_elapsed / 60))
    local total_sec=$((total_elapsed % 60))
    
    # 恢复光标
    tput cnorm
    
    # 根据结果显示
    if [ $exit_code -eq 0 ]; then
        # 清除当前行并打印成功信息
        printf "\r✅ [ %02d:%02d ] %s - 完成\n" "$total_min" "$total_sec" "$message"
    else
        printf "\r❌ [ %02d:%02d ] %s - 失败！\n" "$total_min" "$total_sec" "$message"
        echo "---------------------------------------------------"
        echo "👇 错误日志 (最后 20 行):"
        tail -n 20 "$LOG_FILE"
        echo "---------------------------------------------------"
        echo "完整日志请查看: $LOG_FILE"
        exit 1
    fi
}

echo "🚀 开始构建与打包流程: $APP_NAME"
echo "📄 详细日志将写入: $LOG_FILE"
# 清空之前的日志
> "$LOG_FILE"

# --- 步骤 0: 检查环境 ---
if [ ! -f "$APPIMAGETOOL" ]; then
    echo "❌ 错误: 未找到 $APPIMAGETOOL"
    exit 1
fi

# --- 步骤 1: 编译 ---
# 使用 run_with_timer 包裹耗时命令
run_with_timer "清理旧目录" "rm -rf $BUILD_DIR $APP_DIR"

run_with_timer "CMake 配置 (Release)" \
    "cmake -B $BUILD_DIR -S . -DCMAKE_BUILD_TYPE=Release"

run_with_timer "正在编译 (多核并行)" \
    "cmake --build $BUILD_DIR -j$(nproc)"

if [ ! -f "$BUILD_DIR/$APP_NAME" ]; then
    echo "❌ 严重错误: 未生成可执行文件"
    exit 1
fi

# --- 步骤 2: 准备目录 ---
run_with_timer "建立 AppDir 目录结构" \
    "mkdir -p $APP_DIR/usr/bin && \
     mkdir -p $APP_DIR/usr/share/applications && \
     mkdir -p $APP_DIR/usr/share/icons/hicolor/256x256/apps && \
     mkdir -p $APP_DIR/usr/lib"

# --- 步骤 3: 复制资源 ---
echo "📋 处理资源文件..." 
# 这种瞬间完成的命令不需要加计时器，直接运行即可
cp "$BUILD_DIR/$APP_NAME" "$APP_DIR/usr/bin/"
cp "$ICON_SOURCE" "$APP_DIR/usr/share/icons/hicolor/256x256/apps/${ICON_NAME}.png"
cp "$ICON_SOURCE" "$APP_DIR/${ICON_NAME}.png"

# 生成 .desktop
cat > "$APP_DIR/usr/share/applications/${APP_NAME}.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=$APP_NAME
Exec=$APP_NAME
Icon=$ICON_NAME
Categories=Utility;
Terminal=false
EOF
cp "$APP_DIR/usr/share/applications/${APP_NAME}.desktop" "$APP_DIR/"

# 生成 AppRun
cat > "$APP_DIR/AppRun" <<EOF
#!/bin/bash
HERE="\$(dirname "\$(readlink -f "\${0}")")"
export LD_LIBRARY_PATH="\${HERE}/usr/lib":\$LD_LIBRARY_PATH
export QT_PLUGIN_PATH="\${HERE}/usr/plugins"
export QML2_IMPORT_PATH="\${HERE}/usr/qml"
export XDG_DATA_DIRS="\${HERE}/usr/share":\$XDG_DATA_DIRS
exec "\${HERE}/usr/bin/$APP_NAME" "\$@"
EOF
chmod +x "$APP_DIR/AppRun"

# --- 步骤 4: 抓取依赖 ---
# 这一步通常最慢，加上计时器很有必要
run_with_timer "分析并抓取 Qt 依赖库" \
    "$LINUXDEPLOY $APP_DIR/usr/bin/$APP_NAME -always-overwrite -unsupported-allow-new-glibc -bundle-non-qt-libs"

# --- 步骤 5: 打包 ---
export ARCH=x86_64
run_with_timer "压缩生成 AppImage" \
    "$APPIMAGETOOL $APP_DIR"

echo "🎉 全部完成！文件已生成："
ls -lh ${APP_NAME}-*.AppImage
```

### 安装及卸载

* 将打包生成的可执行文件放在新建的文件夹中，并将 logo.png 也复制到此处
* 新建一个 install.sh 以及 uninstall.sh 脚本
* 然后将这个文件夹复制到需要安装软件的工控机上即可
* 使用 install.sh 时会同时设置开机自启
* 安装和卸载不涉及系统目录下的配置文件以及其他位置的配置文件，只涉及主程序安装目录 /opt/${APP_NAME}，系统快捷菜单目录 /usr/share/applications/${APP_NAME}.desktop，开机自启目录 /$HOME/.config/autostart/${APP_NAME}.desktop

install.sh
```bash
#!/bin/bash

# =================配置区域=================
APP_NAME="RuinapControl"
# 确保这里的文件名和你生成的一致
APP_FILE="RuinapControl-x86_64.AppImage"
ICON_FILE="logo.png"

# 安装目标路径 (工控软件标准目录 /opt)
INSTALL_DIR="/opt/$APP_NAME"

# 系统级快捷方式路径
DESKTOP_FILE_PATH="/usr/share/applications/${APP_NAME}.desktop"
# =========================================

# 检查是否以 root 运行
if [ "$EUID" -ne 0 ]; then
  echo "错误：请以 root 权限运行此脚本 (sudo ./install.sh)"
  exit 1
fi

echo "正在部署 $APP_NAME 到系统目录..."

# 1. 创建 /opt 安装目录
if [ -d "$INSTALL_DIR" ]; then
    echo "发现旧版本，正在清理..."
    rm -rf "$INSTALL_DIR"
fi
mkdir -p "$INSTALL_DIR"

# 2. 复制文件
cp "$APP_FILE" "$INSTALL_DIR/"
cp "$ICON_FILE" "$INSTALL_DIR/"

# 3. 赋予可执行权限
chmod +x "$INSTALL_DIR/$APP_FILE"

echo "文件已安装到: $INSTALL_DIR"

# 4. 生成系统级 .desktop 文件
# 注意：Icon 字段指向了绝对路径，确保图标一定能显示
cat > "$DESKTOP_FILE_PATH" <<EOF
[Desktop Entry]
Type=Application
Name=$APP_NAME
GenericName=Industrial Control Software
Comment=Ruinap Control System
Exec=$INSTALL_DIR/$APP_FILE
Icon=$INSTALL_DIR/$ICON_FILE
Terminal=false
Categories=Utility;Science;
StartupNotify=true
X-GNOME-Autostart-enabled=true
EOF

echo "快捷方式已创建: $DESKTOP_FILE_PATH"

# 5. 刷新系统数据库
# 这一步确保图标立即出现在菜单中，无需注销
if command -v update-desktop-database &> /dev/null; then
    update-desktop-database /usr/share/applications
fi

# --- 自动配置开机自启 (合并版) ---
AUTOSTART_DIR="$HOME/.config/autostart"
if [ ! -d "$AUTOSTART_DIR" ]; then
    mkdir -p "$AUTOSTART_DIR"
fi
cp -f "$DESKTOP_FILE_PATH" "$AUTOSTART_DIR/"
echo "已设置开机自启。"

echo "==========================================="
echo " 安装成功！"
echo " 1. 点击 'Show Applications' 即可找到 $APP_NAME"
echo " 2. 若要固定到侧边栏，请在图标上右键选择 'Add to Favorites'"
echo "==========================================="
```

uninstall.sh
```bash
#!/bin/bash

# =================配置区域=================
APP_NAME="RuinapControl"

# 1. 主程序安装位置 (对应 install.sh 的 /opt/...)
INSTALL_DIR="/opt/$APP_NAME"

# 2. 系统菜单快捷方式 (对应 install.sh 的 /usr/share/applications/...)
DESKTOP_FILE="/usr/share/applications/${APP_NAME}.desktop"

# 3. 开机自启快捷方式 (对应 install.sh 中合并的 autostart 逻辑)
# 使用 $HOME 确保和安装时写入的位置一致
AUTOSTART_FILE="$HOME/.config/autostart/${APP_NAME}.desktop"
# =========================================

# 检查 Root 权限
if [ "$EUID" -ne 0 ]; then
  echo "错误：请以 root 权限运行此脚本 (sudo ./uninstall.sh)"
  exit 1
fi

echo "==========================================="
echo "即将卸载 $APP_NAME"
echo "将会删除以下项目："
echo "  [程序本体] $INSTALL_DIR"
echo "  [菜单图标] $DESKTOP_FILE"
if [ -f "$AUTOSTART_FILE" ]; then
    echo "  [开机自启] $AUTOSTART_FILE"
fi
echo "==========================================="
read -p "确认彻底卸载吗？(y/n): " confirm

if [[ "$confirm" != "y" && "$confirm" != "Y" ]]; then
    echo "操作已取消。"
    exit 0
fi

echo "正在清理..."

# 1. 删除开机自启文件 (最先删除，防止卸载中途断电导致残留)
if [ -f "$AUTOSTART_FILE" ]; then
    rm -f "$AUTOSTART_FILE"
    echo "OK: 已移除开机自启设置"
else
    echo "Pass: 未发现开机自启文件"
fi

# 2. 删除系统菜单快捷方式
if [ -f "$DESKTOP_FILE" ]; then
    rm -f "$DESKTOP_FILE"
    echo "OK: 已移除系统菜单图标"
else
    echo "Pass: 未发现菜单图标"
fi

# 3. 删除主程序目录
if [ -d "$INSTALL_DIR" ]; then
    rm -rf "$INSTALL_DIR"
    echo "OK: 已删除主程序目录"
else
    echo "Pass: 未发现程序目录"
fi

# 4. 刷新系统缓存 (清除图标残留)
if command -v update-desktop-database &> /dev/null; then
    update-desktop-database /usr/share/applications
fi

echo "==========================================="
echo "卸载成功！"
echo "提示：用户的配置文件（如 ~/.config/ 下的数据）已被保留。"
echo "==========================================="
```