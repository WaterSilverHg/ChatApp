#!/bin/bash

# ChatApp Linux 环境安装脚本
# 支持 Ubuntu/Debian 系统

set -e

echo "========================================"
echo "ChatApp Linux 环境安装脚本"
echo "========================================"

# 检测系统
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
else
    echo "无法检测操作系统"
    exit 1
fi

echo "检测到操作系统: $OS"

# ==================== 1. 安装系统依赖 ====================
echo ""
echo "[1/6] 安装系统依赖..."

if [ "$OS" = "ubuntu" ] || [ "$OS" = "debian" ]; then
    sudo apt update
    sudo apt install -y \
        build-essential \
        cmake \
        git \
        pkg-config \
        libssl-dev \
        libpq-dev \
        postgresql-client \
        libhiredis-dev \
        libcurl4-openssl-dev \
        libboost-all-dev \
        libpthread-stubs0-dev \
        zlib1g-dev \
        nlohmann-json3-dev \
        python3 \
        unzip \
        wget \
        curl
elif [ "$OS" = "centos" ] || [ "$OS" = "rhel" ] || [ "$OS" = "fedora" ]; then
    sudo yum install -y \
        gcc \
        gcc-c++ \
        make \
        cmake \
        git \
        openssl-devel \
        postgresql-devel \
        hiredis-devel \
        libcurl-devel \
        boost-devel \
        zlib-devel \
        python3 \
        unzip \
        wget \
        curl
    
    # Fedora 使用 dnf
    if [ "$OS" = "fedora" ]; then
        sudo dnf install -y nlohmann-json-devel
    fi
else
    echo "不支持的操作系统: $OS"
    exit 1
fi

echo "✅ 系统依赖安装完成"

# ==================== 2. 安装 vcpkg ====================
echo ""
echo "[2/6] 安装 vcpkg..."

VCPKG_ROOT="${VCPKG_ROOT:-/opt/vcpkg}"

if [ ! -d "$VCPKG_ROOT" ]; then
    echo "克隆 vcpkg 到 $VCPKG_ROOT..."
    sudo git clone https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT"
    cd "$VCPKG_ROOT"
    sudo ./bootstrap-vcpkg.sh -disableMetrics
else
    echo "vcpkg 已存在于 $VCPKG_ROOT"
fi

echo "✅ vcpkg 安装完成"

# ==================== 3. 安装 vcpkg 依赖 ====================
echo ""
echo "[3/6] 安装 vcpkg 依赖..."

cd "$VCPKG_ROOT"

# 安装 oatpp 相关库
sudo ./vcpkg install oatpp:x64-linux
sudo ./vcpkg install oatpp-websocket:x64-linux
sudo ./vcpkg install oatpp-postgresql:x64-linux
sudo ./vcpkg install oatpp-swagger:x64-linux
sudo ./vcpkg install oatpp-sqlite:x64-linux

# 安装其他依赖
sudo ./vcpkg install poco:x64-linux
sudo ./vcpkg install openssl:x64-linux
sudo ./vcpkg install redis-plus-plus:x64-linux
sudo ./vcpkg install nlohmann-json:x64-linux
sudo ./vcpkg install mailio:x64-linux
sudo ./vcpkg install jwt-cpp:x64-linux

echo "✅ vcpkg 依赖安装完成"

# ==================== 4. 安装 oatpp (手动编译方式，可选) ====================
echo ""
echo "[4/6] 安装 oatpp (手动编译方式)..."

OATPP_VERSION="1.3.0"
OATPP_INSTALL_DIR="/usr/local"

# 检查是否已安装
if [ ! -f "/usr/local/lib64/oatpp-${OATPP_VERSION}/liboatpp.so" ]; then
    echo "手动编译安装 oatpp ${OATPP_VERSION}..."
    
    # oatpp 核心
    cd /tmp
    git clone --depth 1 --branch ${OATPP_VERSION} https://github.com/oatpp/oatpp.git
    cd oatpp
    mkdir build && cd build
    cmake -DCMAKE_INSTALL_PREFIX=${OATPP_INSTALL_DIR} -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc)
    sudo make install
    cd /tmp && rm -rf oatpp
    
    # oatpp-websocket
    cd /tmp
    git clone --depth 1 --branch ${OATPP_VERSION} https://github.com/oatpp/oatpp-websocket.git
    cd oatpp-websocket
    mkdir build && cd build
    cmake -DCMAKE_INSTALL_PREFIX=${OATPP_INSTALL_DIR} -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc)
    sudo make install
    cd /tmp && rm -rf oatpp-websocket
    
    # oatpp-swagger
    cd /tmp
    git clone --depth 1 --branch ${OATPP_VERSION} https://github.com/oatpp/oatpp-swagger.git
    cd oatpp-swagger
    mkdir build && cd build
    cmake -DCMAKE_INSTALL_PREFIX=${OATPP_INSTALL_DIR} -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc)
    sudo make install
    cd /tmp && rm -rf oatpp-swagger
    
    # oatpp-postgresql
    cd /tmp
    git clone --depth 1 --branch ${OATPP_VERSION} https://github.com/oatpp/oatpp-postgresql.git
    cd oatpp-postgresql
    mkdir build && cd build
    cmake -DCMAKE_INSTALL_PREFIX=${OATPP_INSTALL_DIR} -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc)
    sudo make install
    cd /tmp && rm -rf oatpp-postgresql
    
    echo "✅ oatpp 手动编译安装完成"
else
    echo "oatpp 已安装，跳过手动编译"
fi

# ==================== 5. 安装 redis++ (如果 hiredis 已安装但 redis++ 未安装) ====================
echo ""
echo "[5/6] 安装 redis++..."

REDIS_PLUS_PLUS_VERSION="1.3.11"

if [ ! -f "/usr/local/lib/libredis++.so" ]; then
    echo "手动编译安装 redis++ ${REDIS_PLUS_PLUS_VERSION}..."
    
    cd /tmp
    git clone --depth 1 --branch ${REDIS_PLUS_PLUS_VERSION} https://github.com/sewenew/redis-plus-plus.git
    cd redis-plus-plus
    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc)
    sudo make install
    cd /tmp && rm -rf redis-plus-plus
    
    echo "✅ redis++ 安装完成"
else
    echo "redis++ 已安装，跳过"
fi

# ==================== 6. 安装腾讯云 COS SDK ====================
echo ""
echo "[6/6] 安装腾讯云 COS SDK..."

COS_SDK_VERSION="latest"
PROJECT_ROOT=$(dirname "$(realpath "$0")")
COS_LIB_DIR="${PROJECT_ROOT}/ChatServer-http/lib/linux-x64"
COS_INCLUDE_DIR="${PROJECT_ROOT}/ChatServer-http/include/cos-cpp"

# 创建目录
mkdir -p "$COS_LIB_DIR"

# 下载 COS C++ SDK
if [ ! -f "${COS_LIB_DIR}/libcossdk.so" ]; then
    echo "下载腾讯云 COS C++ SDK..."
    
    cd /tmp
    wget -q "https://cos-sdk-archive-1253960454.file.myqcloud.com/cos-cpp-sdk-v5-${COS_SDK_VERSION}.zip" -O cos-sdk.zip || \
    curl -sL "https://cos-sdk-archive-1253960454.file.myqcloud.com/cos-cpp-sdk-v5-${COS_SDK_VERSION}.zip" -o cos-sdk.zip
    
    if [ -f cos-sdk.zip ]; then
        unzip -q cos-sdk.zip
        # 复制库文件
        cp -f cos-cpp-sdk-v5-${COS_SDK_VERSION}/lib/linux/libcossdk.so "${COS_LIB_DIR}/"
        # 复制头文件
        mkdir -p "$COS_INCLUDE_DIR"
        cp -rf cos-cpp-sdk-v5-${COS_SDK_VERSION}/include/* "${COS_INCLUDE_DIR}/"
        rm -rf cos-sdk.zip cos-cpp-sdk-v5-${COS_SDK_VERSION}
        echo "✅ COS SDK 安装完成"
    else
        echo "⚠️ 无法下载 COS SDK，请手动下载: https://cloud.tencent.com/document/product/436/32355"
    fi
else
    echo "COS SDK 已存在，跳过"
fi

# ==================== 更新动态链接库缓存 ====================
echo ""
echo "更新动态链接库缓存..."
sudo ldconfig

# ==================== 输出环境变量建议 ====================
echo ""
echo "========================================"
echo "安装完成！"
echo "========================================"
echo ""
echo "建议设置以下环境变量（添加到 ~/.bashrc）："
echo ""
echo "export VCPKG_ROOT=${VCPKG_ROOT}"
echo "export CMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
echo ""
echo "然后运行: source ~/.bashrc"
echo ""
echo "编译项目命令:"
echo "cd ${PROJECT_ROOT}/ChatServer-http"
echo "mkdir -p build && cd build"
echo "cmake -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake .."
echo "make -j$(nproc)"
echo ""
echo "========================================"