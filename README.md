# ChatApp

一个基于 Oat++ 和 Qt6 的全栈实时聊天应用，**支持分布式部署**，支持私聊、群聊、好友管理、文件上传等功能。

## 项目架构

```
ChatApp/
├── ChatClient/              # Qt6 桌面客户端 (C++17)
├── ChatServer-http/          # REST API 服务器 (Oat++, 端口 8080)
├── ChatServer-websocket/     # WebSocket 实时服务器 (Oat++)
├── ChatServer-sql/           # 数据库初始化脚本 (PostgreSQL)
├── nginx-chatapp.conf        # Nginx 负载均衡配置
└── start-distributed.sh      # 分布式启动脚本
```

### 分布式架构图

```
                        ┌─────────────────────────────────────────┐
                        │           客户端集群                      │
                        │  UserA(IP1)  UserB(IP2)  UserC(IP3)    │
                        └──────────────┬──────────────────────────┘
                                       │
                                       ▼
                        ┌─────────────────────────────────────────┐
                        │              Nginx                       │  ← 负载均衡入口
                        │         (ip_hash 分配)                   │
                        └──────────────┬──────────────────────────┘
                                       │
            ┌──────────────────────────┼──────────────────────────┐
            ▼                          ▼                          ▼
    ┌───────────────┐          ┌───────────────┐          ┌───────────────┐
    │ WS Server 1   │          │ WS Server 2   │          │     Redis     │
    │ 端口: 4567    │          │ 端口: 4568    │          │  Pub/Sub      │
    │ 用户: A,C     │          │ 用户: B,D     │          │               │
    └───────┬───────┘          └───────┬───────┘          └───────┬───────┘
            │                          │                          │
            └──────────────────────────┴──────────────────────────┘
                      跨服务器消息同步 (Redis Pub/Sub)
```

### 分布式消息流程

```
用户A (Server 1) 发送消息给用户B (Server 2)

UserA ──> Server 1 ──> 本地查找 UserB
                            │
                            │ B 不在本地
                            ▼
                      发布到 Redis
                      chatapp:private_message
                      {"serverId": "server_1", "targetUserUuid": "B", "payload": "..."}
                            │
                            ▼
                      Redis Pub/Sub
                            │
                            │ 所有服务器都订阅了这个频道
                            ▼
              Server 1 (忽略自己发送的消息)    Server 2 (收到消息)
                                                  │
                                                  ▼
                                            查找本地用户 B
                                                  │
                                                  ▼
                                            B 在线 ──> 发送消息给 B
```

## 技术栈

| 组件 | 技术 | 说明 |
|------|------|------|
| HTTP 框架 | Oat++ 1.3.0 | 高性能 Web 框架 |
| WebSocket | oatpp-websocket | 实时消息推送 |
| 数据库 | PostgreSQL (pgcrypto + uuid-ossp) | 主数据库 |
| ORM | oatpp-postgresql | 数据库访问 |
| 缓存 / 分布式协调 | Redis (redis++) | Session、Pub/Sub 跨服务器通信 |
| 认证 | JWT (jwt-cpp, HS256) + Redis Session | 安全认证 |
| 密码加密 | bcrypt | 用户密码加密 |
| 邮件 | mailio (SMTP) | 验证码发送 |
| 文件存储 | 腾讯云 COS SDK | 头像、文件存储 |
| JSON 解析 | picojson / oatpp parser-json | 数据序列化 |
| 客户端 | Qt 6.8 + QWebSocket | 桌面客户端 |
| 负载均衡 | Nginx | 分布式入口 |
| 构建 | CMake (服务端) / qmake (客户端) | 项目构建 |

## 功能列表

### 用户系统
- 邮箱注册 / 登录
- JWT Token 认证 (3天过期)
- Redis Session 管理 (注销即时失效)
- 密码重置 (邮箱验证码)
- 在线状态 (online / offline / away / busy)

### 好友系统
- 好友请求发送 / 接受 / 拒绝
- 好友删除
- 拉黑 / 取消拉黑
- 好友备注 / 分组

### 群组系统
- 创建 / 更新 / 解散群组
- 成员添加 / 移除
- 角色管理 (owner / admin / member)
- 入群申请处理
- 群主退出时自动转让或解散

### 消息系统
- 私聊消息 (在线实时推送 + 离线未读计数)
- 群聊消息 (全员推送，跨服务器支持)
- 消息撤回
- 历史消息分页加载 (可分页 / 游标查询)
- 会话列表 (未读计数、最后消息)
- 消息类型支持: text / image / file / audio / video

### 文件系统
- 文件上传到腾讯云 COS
- 头像上传
- 上传状态跟踪 (uploading / completed / failed)

### 客户端特性
- 会话列表实时更新
- 上拉加载更多历史消息
- 消息本地缓存 (JSON 文件)
- 好友 / 群组请求管理

## API 接口一览

### REST API (端口 8080)

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| POST | `/api/auth/login` | 登录 | 否 |
| POST | `/api/auth/register` | 注册 | 否 |
| POST | `/api/auth/logout` | 注销 (WebSocket 处理) | 是 |
| POST | `/api/auth/reset-password` | 重置密码 | 否 |
| POST | `/api/auth/send-code` | 发送验证码 | 否 |
| GET | `/api/messages/private/{uuid}/page?page=&size=` | 私聊消息分页 | 是 |
| GET | `/api/messages/private/{uuid}/before?id=&limit=` | 私聊消息游标 | 是 |
| GET | `/api/messages/group/{uuid}/page?page=&size=` | 群聊消息分页 | 是 |
| GET | `/api/messages/group/{uuid}/before?id=&limit=` | 群聊消息游标 | 是 |
| POST | `/api/files/record` | 创建上传记录 | 是 |
| POST | `/api/files/upload` | 上传文件数据 | 是 |
| GET | `/api/files/{uuid}` | 获取文件详情 | 是 |
| GET | `/api/friends` | 获取好友列表 | 是 |
| GET | `/api/friends/requests/sent` | 发出的好友请求 | 是 |
| GET | `/api/friends/requests/received` | 收到的好友请求 | 是 |
| GET | `/api/groups` | 我的群组列表 | 是 |
| GET | `/api/groups/{uuid}` | 群组详情 | 是 |
| GET | `/api/groups/{uuid}/members` | 群成员列表 | 是 |
| GET | `/api/groups/search?keyword=` | 搜索群组 | 是 |
| GET | `/api/users/search?keyword=` | 搜索用户 | 是 |
| GET | `/api/conversations` | 会话列表 | 是 |
| PUT | `/api/users/status` | 更新在线状态 | 是 |

### WebSocket 消息协议 (通过 Nginx 访问)

**请求 (客户端→服务端)**：

| type | 说明 |
|------|------|
| `send_friend_request` | 发送好友请求 |
| `handle_friend_request` | 处理好友请求 |
| `delete_friend` | 删除好友 |
| `block_user` | 拉黑用户 |
| `unblock_user` | 取消拉黑 |
| `create_group` | 创建群组 |
| `update_group` | 更新群组信息 |
| `dissolve_group` | 解散群组 |
| `add_group_members` | 添加群成员 |
| `remove_group_member` | 移除群成员 |
| `set_member_role` | 设置成员角色 |
| `join_group` | 加入群组 |
| `leave_group` | 退出群组 |
| `send_group_request` | 发送入群申请 |
| `handle_group_request` | 处理入群申请 |
| `send_private_message` | 发送私聊消息 |
| `send_group_message` | 发送群聊消息 |
| `recall_message` | 撤回消息 |

**响应 (服务端→客户端)**：

| type | 说明 |
|------|------|
| `send_private_message_response` | 私聊消息发送结果 |
| `send_group_message_response` | 群聊消息发送结果 |
| `new_private_message` | 新私聊消息推送 |
| `new_group_message` | 新群聊消息推送 |
| `send_friend_request_response` | 好友请求发送结果 |
| `*_response` | 各操作对应的响应 |
| `error` | 错误消息 |

WebSocket 消息使用 JSON 格式，通过 `"type"` 字段路由到对应处理器。

## Redis Pub/Sub 频道设计

| 频道名 | 用途 | DTO |
|--------|------|-----|
| `chatapp:private_message` | 私聊消息跨服务器转发 | DistributedMessageDTO |
| `chatapp:group_message` | 群聊消息跨服务器转发 | DistributedGroupMessageDTO |
| `chatapp:user_status` | 用户状态同步 | DistributedStatusDTO |
| `chatapp:friend_request` | 好友请求同步 | DistributedFriendRequestDTO |
| `chatapp:group_event` | 群组事件同步 | DistributedGroupEventDTO |

## 数据库表结构

| 表名 | 说明 | 关键字段 |
|------|------|----------|
| `users` | 用户表 | uuid, username, email, password_hash(bcrypt), status |
| `user_settings` | 用户设置 | theme, language, notification_enabled |
| `friendships` | 好友关系 | user_id, friend_id, status(pending/accepted/block/blocked/deleted) |
| `friend_requests` | 好友请求 | uuid, from_user_id, to_user_id, message, status |
| `groups` | 群组 | uuid, name, owner_id, max_members, is_public |
| `group_members` | 群成员 | group_id, user_id, role(owner/admin/member) |
| `group_requests` | 入群申请 | uuid, group_id, requester_id, status |
| `messages` | 消息 | uuid, from_user_id, to_user_id/to_group_id, message_type, content |
| `conversations` | 会话 | user_id, target_user_id/target_group_id, unread_count |
| `files` | 文件 | uuid, user_id, file_name, file_path, upload_status |

软删除通过 `deleted_at` 时间戳字段实现，所有可变表带 `updated_at` 自动更新触发器。

## 快速开始

### 前置依赖

- **Windows**: Visual Studio 2022, vcpkg, Qt 6.8
- **Linux**: GCC/Clang (C++17), CMake 3.16+, Qt 6.x (客户端), Nginx

**服务端依赖 (通过 vcpkg)**：

```bash
vcpkg install oatpp:x64-windows \
    oatpp-websocket:x64-windows \
    oatpp-postgresql:x64-windows \
    oatpp-swagger:x64-windows \
    poco:x64-windows \
    openssl:x64-windows \
    redis-plus-plus:x64-windows \
    nlohmann-json:x64-windows \
    mailio:x64-windows \
    jwt-cpp:x64-windows
```

**外部库 (项目内 `lib/` 目录)**：
- bcrypt (预编译 `.lib`)
- 腾讯云 COS SDK (`cos-cpp`)

### 1. 数据库初始化

```bash
# 确保 PostgreSQL 运行中
psql -U postgres -f ChatServer-sql/psqlinit1.sql
psql -U postgres -d chatroom -f ChatServer-sql/psqlinit2.sql
```

这将创建 `chatroom` 数据库及所有表、索引、触发器。

### 2. Redis 启动

```bash
# 默认端口 6379
redis-server
```

### 3. 配置文件

在 `ChatServer-http/` 和 `ChatServer-websocket/` 目录下各创建 `config.json`：

#### HTTP 服务器配置 (`ChatServer-http/config.json`)

```json
{
  "Pagination": 20,
  "server_host": "0.0.0.0",
  "server_port": 8080,
  "postgresql_connection_string": "host=localhost port=5432 dbname=chatroom user=postgres password=your_db_password",
  "redis_url": "tcp://127.0.0.1:6379",
  "jwt_secret": "your-jwt-secret-key-change-in-production",
  "jwt_issuer": "chatapp-server",
  "smtp_host": "smtp.qq.com",
  "smtp_port": 465,
  "sender_name": "ChatApp",
  "sender_email": "your_email@qq.com",
  "sender_password": "your_smtp_authorization_code",
  "cos_app_id": "your_cos_app_id",
  "cos_secret_id": "your_cos_secret_id",
  "cos_secret_key": "your_cos_secret_key",
  "cos_region": "ap-guangzhou",
  "cos_bucket_name": "your-bucket-name"
}
```

#### WebSocket 服务器配置 (`ChatServer-websocket/config.json`)

```json
{
  "Pagination": 20,
  "server_host": "0.0.0.0",
  "server_port": 4567,
  "postgresql_connection_string": "host=localhost port=5432 dbname=chatroom user=postgres password=your_db_password",
  "redis_url": "tcp://127.0.0.1:6379",
  "jwt_secret": "your-jwt-secret-key-change-in-production",
  "jwt_issuer": "chatapp-server",
  "smtp_host": "smtp.qq.com",
  "smtp_port": 465,
  "sender_name": "ChatApp",
  "sender_email": "your_email@qq.com",
  "sender_password": "your_smtp_authorization_code"
}
```

> **注意**: 所有服务器的 `jwt_secret` 必须一致！

### 4. 编译与运行

#### 服务端 (Windows)

```bash
# HTTP 服务器
cd ChatServer-http
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/cppsoft/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build .

# WebSocket 服务器
cd ChatServer-websocket
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/cppsoft/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build .
```

#### 客户端 (Windows)

```bash
cd ChatClient
qmake ChatClient.pro
# 或在 Qt Creator 中打开 ChatClient.pro 直接编译运行
```

## 分布式部署

### 1. Nginx 负载均衡配置

使用项目提供的 `nginx-chatapp.conf` 配置文件：

```bash
# 复制配置文件到 Nginx 目录
sudo cp nginx-chatapp.conf /etc/nginx/sites-available/chatapp
sudo ln -s /etc/nginx/sites-available/chatapp /etc/nginx/sites-enabled/

# 检查配置并重启 Nginx
sudo nginx -t
sudo systemctl restart nginx
```

### 2. 启动多个 WebSocket 服务器

修改 `nginx-chatapp.conf` 中的上游服务器配置：

```nginx
upstream websocket_servers {
    ip_hash;  # 基于客户端 IP 哈希分配
    server 127.0.0.1:4567;  # Server 1
    server 127.0.0.1:4568;  # Server 2
    # server 192.168.1.100:4567;  # 分布式部署时使用实际 IP
}
```

### 3. 使用启动脚本

```bash
chmod +x start-distributed.sh
./start-distributed.sh
```

该脚本会自动启动：
- 2 个 HTTP 服务器实例
- 2 个 WebSocket 服务器实例
- Nginx 负载均衡器

### 4. 客户端配置

客户端连接地址在 [ChatClient/src/global.h](ChatClient/src/global.h) 中定义：

```cpp
// 开发环境（连接到 Nginx）
static const QString HTTP_BASE_URL = "http://127.0.0.1:8080";
static const QString WEBSOCKET_URL = "ws://127.0.0.1:8080/ws";

// 生产环境（使用 HTTPS/WSS）
// static const QString HTTP_BASE_URL = "https://your-domain.com";
// static const QString WEBSOCKET_URL = "wss://your-domain.com/ws";
```

## 生产部署建议

### TLS 加密

建议通过 Nginx 反向代理添加 TLS（已在 `nginx-chatapp.conf` 中配置）。

### 配置安全性

- `config.json` 不应提交到版本控制，已在根目录 `.gitignore` 中排除
- 生产环境更换 `jwt_secret` 为高强度随机字符串
- SMTP 密码使用邮箱授权码而非登录密码
- COS 密钥建议使用子账号密钥，仅授予必要的 COS 权限

### 构建选项

- `SWAGGER`: 启用 Swagger API 文档 (`-DSWAGGER` 编译宏)
- `SQLCHECK`: 调试模式下返回数据库原始错误 (默认启用，生产环境建议去除)

## 项目目录详解

```
ChatServer-http/src/
├── main.cpp                     # 入口点
├── global.h                     # 全局包含
├── config/
│   └── SettingController.cpp/.h # JSON 配置文件读取
├── jwt/
│   └── Appjwt.cpp/.h            # JWT 创建/验证
├── redis/
│   └── AppRedis.hpp             # Redis 封装 (Session/验证码)
├── cos/
│   └── AppCos.hpp               # 腾讯云 COS 封装
├── smtp/
│   └── AppEmail.hpp             # SMTP 邮件发送
├── tool/
│   └── RandomGenerator.hpp      # 随机字符串生成
└── server/
    ├── server.cpp               # 组装组件、注册路由
    ├── component/               # Oat++ IOC 组件
    ├── controller/              # API 控制器 (路由定义)
    ├── dto/                     # 数据传输对象
    ├── vo/                      # 视图对象
    ├── service/                 # 业务逻辑层
    ├── handler/                 # 认证/错误处理
    └── postgresql/
        └── AppClient.hpp        # 数据库查询 (~100 QUERY 宏)
```

```
ChatServer-websocket/src/
├── (同上结构)
├── redis/
│   ├── AppRedis.hpp             # Redis 封装
│   └── RedisPubSubManager.hpp   # Redis Pub/Sub 分布式消息管理
├── websocket/
│   ├── AppWebSocket.hpp         # WebSocket 连接管理器
│   └── AppWebSocket.cpp         # WebSocket 方法实现（避免循环依赖）
└── server/
    ├── handler/
    │   ├── SharedWebSocketResources.hpp  # 消息分发中心
    │   └── LightweightAsyncWebSocketListener.hpp  # 心跳 + 消息接收
    └── coroutine/
        └── Coroutines.hpp       # 异步协程 (Ping/Pong/Close/Send)
```

## 认证流程

```
1. 客户端 → POST /api/auth/login (email + password)
2. 服务端 → 验证密码 → 创建 JWT(payload: userUuid + sessionId) → Redis 存储 session
3. 客户端 → 保存 token, 建立 WebSocket 连接
4. 客户端 → GET /ws + Header: Authorization: Bearer <token>
5. Nginx → 基于 IP 哈希路由到特定 WebSocket 服务器
6. AppAuthInterceptor → 验证 JWT → 校验 Redis session → 提取 userUuid
7. WebSocket 连接建立 → 用户状态设为 online → 订阅 Redis 频道
8. WebSocket 断开 → Redis session 删除 → 用户状态设为 offline
```

JWT 过期时间: 3 天 (`EXPIRESIN = 3600 * 24 * 3`, 在 [global.h](ChatServer-http/src/global.h) 中定义)

## License

本项目仅供学习使用。

## Docker 部署

### 快速启动 (Docker Compose)

项目提供了完整的 Docker 配置，采用**三阶段构建**（依赖安装 → 编译 → 轻量运行时镜像），可一键部署所有服务端组件。

#### 1. 准备配置文件

```bash
# 复制环境变量模板（仅用于 PostgreSQL 密码）
cp .env.example .env

# 编辑 .env 文件，设置数据库密码
vim .env

# 创建 HTTP 服务器配置
cat > ChatServer-http/config.json << EOF
{
  "Pagination": 20,
  "server_host": "0.0.0.0",
  "server_port": 8080,
  "postgresql_connection_string": "host=postgres port=5432 dbname=chatroom user=postgres password=your_db_password",
  "redis_url": "tcp://redis:6379",
  "jwt_secret": "your-jwt-secret-key-change-in-production",
  "jwt_issuer": "chatapp-server",
  "smtp_host": "smtp.qq.com",
  "smtp_port": 465,
  "sender_name": "ChatApp",
  "sender_email": "your_email@qq.com",
  "sender_password": "your_smtp_authorization_code",
  "cos_app_id": "your_cos_app_id",
  "cos_secret_id": "your_cos_secret_id",
  "cos_secret_key": "your_cos_secret_key",
  "cos_region": "ap-guangzhou",
  "cos_bucket_name": "your-bucket-name"
}
EOF

# 创建 WebSocket 服务器配置 (不需要 COS 配置)
cat > ChatServer-websocket/config.json << EOF
{
  "Pagination": 20,
  "server_host": "0.0.0.0",
  "server_port": 4567,
  "postgresql_connection_string": "host=postgres port=5432 dbname=chatroom user=postgres password=your_db_password",
  "redis_url": "tcp://redis:6379",
  "jwt_secret": "your-jwt-secret-key-change-in-production",
  "jwt_issuer": "chatapp-server",
  "smtp_host": "smtp.qq.com",
  "smtp_port": 465,
  "sender_name": "ChatApp",
  "sender_email": "your_email@qq.com",
  "sender_password": "your_smtp_authorization_code"
}
EOF
```

> **重要**:
> - 所有服务器的 `jwt_secret` 必须一致！
> - `.env` 文件**仅用于 PostgreSQL 容器密码**，服务器运行时配置通过 `config.json` 设置
> - `config.json` 会通过 Docker 卷挂载到容器内部

#### 2. 启动服务

```bash
# 构建并启动所有服务
docker-compose up -d

# 查看服务状态
docker-compose ps

# 查看日志
docker-compose logs -f http-server
docker-compose logs -f websocket-server
```

**首次构建说明**：
- 首次构建会在容器内克隆 vcpkg 并安装所有依赖，**大约需要 15-30 分钟**
- 后续构建会利用 Docker 缓存层，仅重新编译代码，速度会大幅提升

**使用宿主机 vcpkg 加速构建**（可选）：
```bash
# Windows 系统（WSL2 路径，替换为你的 vcpkg 实际路径）
docker build --build-arg VCPKG_HOST_PATH=/path/to/vcpkg -f ChatServer-http/Dockerfile -t chatapp-http .

# Linux 系统（替换为你的 vcpkg 实际路径）
docker build --build-arg VCPKG_HOST_PATH=/path/to/vcpkg -f ChatServer-http/Dockerfile -t chatapp-http .
```

通过 `VCPKG_HOST_PATH` 参数可以复用宿主机的 vcpkg 安装，大幅缩短构建时间（从 15-30 分钟降至 5-10 分钟）。

#### 3. 服务说明

Docker Compose 会启动以下服务：

| 服务 | 容器名 | 端口 | 说明 |
|------|--------|------|------|
| `postgres` | chatapp-postgres | 5432 | PostgreSQL 数据库 |
| `redis` | chatapp-redis | 6379 | Redis 缓存和 Pub/Sub |
| `http-server` | chatapp-http | 8080 | REST API 服务器 |
| `websocket-server` | chatapp-websocket | 4567 | WebSocket 实时服务器 |

#### 4. 停止服务

```bash
# 停止所有服务
docker-compose down

# 停止并删除数据卷 (清除数据库数据)
docker-compose down -v

# 强制重新构建（更新依赖时使用）
docker-compose build --no-cache
```

### 三阶段构建说明

项目采用 Docker **三阶段构建**优化镜像体积和构建效率：

```
阶段1: builder-vcpkg          阶段2: builder-app          阶段3: runtime
┌───────────────────────┐    ┌───────────────────────┐    ┌───────────────────────┐
│ 克隆 vcpkg            │    │ 复制项目代码          │    │ 仅包含运行时依赖      │
│ 安装所有依赖          │    │ 编译项目              │    │ 复制编译产物          │
│ (体积约 8GB+)         │    │ (继承依赖层)          │    │ (体积约 100MB)        │
└───────────────────────┘    └───────────────────────┘    └───────────────────────┘
         │                            │                            │
         └───────────┬────────────────┘                            │
                     ▼                                             │
              Docker 缓存层 ◄─────────────────────────────────────┘
              (依赖不变时跳过)
```

**优势**：
- 运行时镜像体积小（约 100MB），启动快
- 依赖安装层可缓存，后续构建只编译代码
- 不依赖宿主机的 vcpkg，跨平台构建一致

### 单独构建镜像

如果只需要构建单个服务镜像：

```bash
# 构建 HTTP 服务器
docker build -f ChatServer-http/Dockerfile -t chatapp-http .

# 构建 WebSocket 服务器
docker build -f ChatServer-websocket/Dockerfile -t chatapp-websocket .
```

### Docker 部署注意事项

1. **数据库初始化**: Docker Compose 会自动执行 SQL 初始化脚本
2. **数据持久化**: 数据库和 Redis 数据存储在 Docker 卷中
3. **配置安全**: 不要将 `config.json` 和 `.env` 文件提交到版本控制
4. **资源限制**: 可在 `docker-compose.yml` 中添加 `deploy.resources` 限制内存/CPU
5. **构建缓存**: 修改 `vcpkg.json` 会触发重新安装所有依赖，耗时较长

## 分布式部署验证

启动后，可以通过以下方式验证分布式功能：

1. **连接测试**: 使用不同 IP 的客户端连接到 Nginx
2. **消息转发测试**: 用户 A 发送消息给用户 B（连接到不同服务器）
3. **状态同步测试**: 用户上线/下线状态应正确同步
4. **日志验证**: 检查各服务器日志确认跨服务器消息传递

分布式技术实现总结：

| 组件 | 技术 | 实现方式 |
|------|------|----------|
| 负载均衡 | Nginx | ip_hash 确保同一用户连接到同一服务器 |
| 消息路由 | Redis Pub/Sub | 跨服务器消息广播和转发 |
| 去重机制 | Server ID | 每个服务器生成唯一 ID，忽略自己发送的消息 |
| 状态管理 | Redis + PostgreSQL | 共享状态存储 |