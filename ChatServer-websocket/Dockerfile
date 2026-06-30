# ========================================
# ChatApp WebSocket Server Dockerfile
# 三阶段构建：依赖安装 → 编译 → 运行时镜像
# ========================================

# 阶段1：安装 vcpkg 和依赖
FROM ubuntu:22.04 AS builder-vcpkg

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

# 支持从宿主机复制 vcpkg（Windows/Linux 路径兼容）
# 构建时通过 --build-arg VCPKG_HOST_PATH=<宿主机路径> 指定
ARG VCPKG_HOST_PATH=none

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

WORKDIR /opt

# 优先尝试从宿主机复制 vcpkg
RUN if [ "$VCPKG_HOST_PATH" != "none" ] && [ -d "$VCPKG_HOST_PATH" ]; then \
      echo "✅ 使用宿主机 vcpkg: $VCPKG_HOST_PATH"; \
      cp -r "$VCPKG_HOST_PATH" /opt/vcpkg; \
    else \
      echo "⚠️  宿主机 vcpkg 不存在，在容器内克隆安装"; \
      git clone https://github.com/microsoft/vcpkg.git && \
      cd vcpkg && \
      ./bootstrap-vcpkg.sh -disableMetrics; \
    fi

# 确保 vcpkg 已就绪
RUN cd /opt/vcpkg && \
    if [ ! -f vcpkg ]; then \
      echo "❌ vcpkg bootstrap 失败，重新尝试"; \
      ./bootstrap-vcpkg.sh -disableMetrics; \
    fi

ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

COPY vcpkg.json /opt/vcpkg/

WORKDIR /opt/vcpkg
RUN ./vcpkg install --triplet x64-linux --clean-after-build

# 阶段2：编译项目
FROM builder-vcpkg AS builder-app

WORKDIR /app

COPY ChatServer-websocket /app/ChatServer-websocket

WORKDIR /app/ChatServer-websocket/build
RUN cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DOATPP_VERSION=1.3.1 \
    && cmake --build . --config Release -j$(nproc)

# 阶段3：运行时镜像（轻量）
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

RUN apt-get update && apt-get install -y \
    libssl3 \
    libpq5 \
    libhiredis0.14 \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /app/bin

COPY --from=builder-app /app/ChatServer-websocket/build/bin/ChatServer /app/bin/
COPY --from=builder-app /app/ChatServer-websocket/config.json /app/bin/config.json.template

EXPOSE 4567

WORKDIR /app/bin

CMD ["/app/bin/ChatServer"]