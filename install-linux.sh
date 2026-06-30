#!/bin/bash

# ChatApp Linux 环境安装脚本
# 支持 Ubuntu/Debian 系统
# 仅使用 vcpkg 安装依赖，避免重复编译

set -e

# 获取项目根目录（脚本所在目录）
PROJECT_ROOT=$(dirname "$(realpath "$0")")

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
echo "[1/4] 安装系统依赖..."

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
echo "[2/4] 安装 vcpkg..."

VCPKG_ROOT="${VCPKG_ROOT:-/opt/vcpkg}"

if [ ! -d "$VCPKG_ROOT" ]; then
    echo "克隆 vcpkg 到 $VCPKG_ROOT..."
    sudo git clone https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT"
    cd "$VCPKG_ROOT"
    sudo ./bootstrap-vcpkg.sh -disableMetrics
else
    echo "vcpkg 已存在于 $VCPKG_ROOT，更新..."
    cd "$VCPKG_ROOT"
    sudo git pull
fi

echo "✅ vcpkg 安装完成"

# ==================== 3. 通过 vcpkg 安装所有依赖 ====================
echo ""
echo "[3/4] 通过 vcpkg 安装所有依赖..."

# 确认 vcpkg.json 存在
if [ ! -f "$PROJECT_ROOT/vcpkg.json" ]; then
    echo "❌ 错误: 未找到 vcpkg.json 文件"
    echo "   期望路径: $PROJECT_ROOT/vcpkg.json"
    exit 1
fi

echo "找到 manifest 文件: $PROJECT_ROOT/vcpkg.json"

# Manifest mode: 直接在项目根目录运行 vcpkg install
# vcpkg 会自动检测当前目录的 vcpkg.json 文件
cd "$PROJECT_ROOT"

# 使用 vcpkg install，它会在当前目录自动检测 vcpkg.json
# 注意：需要使用 --classic 或确保 vcpkg 不去其他目录查找
"$VCPKG_ROOT/vcpkg" install

echo "✅ vcpkg 依赖安装完成"

# ==================== 4. 安装腾讯云 COS SDK ====================
echo ""
echo "[4/4] 安装腾讯云 COS SDK..."

COS_SDK_VERSION="latest"
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
echo "所有依赖已通过 vcpkg 安装到:"
echo "  ${VCPKG_ROOT}/installed/x64-linux/"
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