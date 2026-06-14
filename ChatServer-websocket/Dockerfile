# ChatApp WebSocket Server Dockerfile
# 基于 Ubuntu 22.04 构建

FROM ubuntu:22.04

# 设置环境变量
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

# 安装基础依赖
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libssl-dev \
    libpq-dev \
    libhiredis-dev \
    curl \
    wget \
    unzip \
    zip \
    ninja-build \
    && rm -rf /var/lib/apt/lists/*

# 复制预下载的 vcpkg（已在宿主机上运行 bootstrap-vcpkg.sh）
COPY vcpkg /opt/vcpkg

# 设置 vcpkg 环境变量
ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

# 复制 vcpkg manifest 文件
COPY vcpkg.json /opt/vcpkg/

# 使用 manifest 模式安装依赖
WORKDIR /opt/vcpkg
RUN ./vcpkg install --triplet x64-linux

# 复制项目代码（include/ 目录包含外部库 bcrypt, jwt-cpp 等）
WORKDIR /app
COPY ChatServer-websocket /app/ChatServer-websocket

# 构建 WebSocket 服务器
WORKDIR /app/ChatServer-websocket
RUN mkdir -p build && cd build \
    && cmake .. -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
    && cmake --build . --config Release

# 暴露端口
EXPOSE 4567

# 启动命令（使用绝对路径）
CMD ["/app/ChatServer-websocket/build/bin/ChatServer-websocket"]