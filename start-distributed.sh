#!/bin/bash

# ========================================
# ChatApp 分布式部署启动脚本
# 启动多个 WebSocket 服务器和 Nginx 负载均衡
# ========================================

set -e

echo "========================================"
echo "ChatApp 分布式部署启动脚本"
echo "========================================"

# 配置参数
HTTP_SERVER_PORT_1=8081
HTTP_SERVER_PORT_2=8082
WS_SERVER_PORT_1=4567
WS_SERVER_PORT_2=4568

# 创建日志目录
mkdir -p logs

echo ""
echo "[1/3] 启动 HTTP 服务器..."

# 启动第一个 HTTP 服务器
cd ChatServer-http/build || {
    echo "❌ 未找到 HTTP 服务器构建目录"
    exit 1
}

./ChatServer --http-port=$HTTP_SERVER_PORT_1 > ../../logs/http_server_1.log 2>&1 &
HTTP_PID_1=$!
echo "✅ HTTP Server 1 启动成功 (端口: $HTTP_SERVER_PORT_1, PID: $HTTP_PID_1)"

# 启动第二个 HTTP 服务器
./ChatServer --http-port=$HTTP_SERVER_PORT_2 > ../../logs/http_server_2.log 2>&1 &
HTTP_PID_2=$!
echo "✅ HTTP Server 2 启动成功 (端口: $HTTP_SERVER_PORT_2, PID: $HTTP_PID_2)"

sleep 2

echo ""
echo "[2/3] 启动 WebSocket 服务器..."

cd ../../ChatServer-websocket/build || {
    echo "❌ 未找到 WebSocket 服务器构建目录"
    exit 1
}

# 启动第一个 WebSocket 服务器
./ChatServer-websocket --port=$WS_SERVER_PORT_1 > ../../logs/ws_server_1.log 2>&1 &
WS_PID_1=$!
echo "✅ WebSocket Server 1 启动成功 (端口: $WS_SERVER_PORT_1, PID: $WS_PID_1)"

# 启动第二个 WebSocket 服务器
./ChatServer-websocket --port=$WS_SERVER_PORT_2 > ../../logs/ws_server_2.log 2>&1 &
WS_PID_2=$!
echo "✅ WebSocket Server 2 启动成功 (端口: $WS_SERVER_PORT_2, PID: $WS_PID_2)"

sleep 2

echo ""
echo "[3/3] 启动 Nginx 负载均衡..."

# 检查 Nginx 配置
if [ ! -f "../nginx-chatapp.conf" ]; then
    echo "❌ 未找到 Nginx 配置文件"
    exit 1
fi

# 启动 Nginx（使用自定义配置）
nginx -c "$(pwd)/../nginx-chatapp.conf"
echo "✅ Nginx 负载均衡启动成功"

echo ""
echo "========================================"
echo "部署完成！"
echo "========================================"
echo ""
echo "服务状态:"
echo "- HTTP Server 1: http://localhost:$HTTP_SERVER_PORT_1"
echo "- HTTP Server 2: http://localhost:$HTTP_SERVER_PORT_2"
echo "- WebSocket Server 1: ws://localhost:$WS_SERVER_PORT_1/ws"
echo "- WebSocket Server 2: ws://localhost:$WS_SERVER_PORT_2/ws"
echo "- Nginx 入口: http://localhost:8080 (HTTP) / https://localhost (HTTPS)"
echo ""
echo "PID 信息:"
echo "- HTTP Server 1: $HTTP_PID_1"
echo "- HTTP Server 2: $HTTP_PID_2"
echo "- WebSocket Server 1: $WS_PID_1"
echo "- WebSocket Server 2: $WS_PID_2"
echo ""
echo "停止服务命令:"
echo "kill $HTTP_PID_1 $HTTP_PID_2 $WS_PID_1 $WS_PID_2 && nginx -s stop"
echo ""
echo "日志文件位于: logs/ 目录"
echo "========================================"

# 保存 PID 到文件
echo "$HTTP_PID_1" > logs/http_server_1.pid
echo "$HTTP_PID_2" > logs/http_server_2.pid
echo "$WS_PID_1" > logs/ws_server_1.pid
echo "$WS_PID_2" > logs/ws_server_2.pid