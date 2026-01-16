# 基础镜像：使用 Ubuntu 22.04
# 注意：在 M2 Mac 上构建时，默认会拉取 arm64 架构的 Ubuntu，这对本机调试最快。
FROM ubuntu:22.04

# 避免交互式弹窗
ENV DEBIAN_FRONTEND=noninteractive

# 替换 apt 源为阿里云（可选，为了国内下载速度）
RUN sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list

# 安装 C++ 编译环境、CMake 和 Qt5/Qt6 库
# 这里以 Qt5 为例，工控机常用。如果需要 Qt6 请替换为 qt6-base-dev
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    gdb \
    git \
    qtbase5-dev \
    qt5-qmake \
    libqt5serialport5-dev \
    libqt5svg5-dev \
    libqt5websockets5-dev \
    mesa-utils \
    libgl1-mesa-dev \
    fonts-wqy-microhei \
    && rm -rf /var/lib/apt/lists/*

# 设置工作目录
WORKDIR /app

# 默认命令
CMD ["/bin/bash"]