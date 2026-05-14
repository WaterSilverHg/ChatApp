# ChatApp

一个基于 Oat++ 和 Qt6 的全栈实时聊天应用，支持私聊、群聊、好友管理、文件上传等功能。

## 项目架构

```
ChatApp/
├── ChatClient/              # Qt6 桌面客户端 (C++17)
├── ChatServer-http/          # REST API 服务器 (Oat++, 端口 8080)
├── ChatServer-websocket/     # WebSocket 实时服务器 (Oat++, 端口 4567)
└── ChatServer-sql/           # 数据库初始化脚本 (PostgreSQL)
```

```
                        ┌─────────────────────┐
                        │   ChatClient (Qt6)   │
                        │   桌面 GUI 客户端      │
                        └──────┬──────┬────────┘
                               │      │
              HTTP REST        │      │  WebSocket 实时
          http://127.0.0.1:8080│      │ ws://127.0.0.1:4567/ws
                               │      │
        ┌──────────────────────v──┐ ┌─v──────────────────────────┐
        │  ChatServer-http (Oat++)│ │ ChatServer-websocket (Oat++) │
        │  - 登录/注册             │ │ - 实时消息推送               │
        │  - 历史消息分页          │ │ - 好友/群组操作              │
        │  - 文件上传              │ │ - 在线状态管理               │
        │  - 用户搜索              │ │ - 心跳检测                   │
        └──────────┬──────────────┘ └─┬────────────────────────────┘
                   │                  │
                   └──────┬───────────┘
                          │
          ┌───────────────v────────────────┐
          │         PostgreSQL             │
          │  数据库: chatroom               │
          └───────────────┬────────────────┘
                          │
          ┌───────────────v────────────────┐
          │     Redis + Tencent COS        │
          │  Session / 验证码  │ 文件/头像  │
          └────────────────────────────────┘
```

## 技术栈

| 组件 | 技术 |
|------|------|
| HTTP 框架 | Oat++ 1.3.0 |
| WebSocket | oatpp-websocket |
| 数据库 | PostgreSQL (pgcrypto + uuid-ossp) |
| ORM | oatpp-postgresql |
| 缓存 | Redis (redis++) |
| 认证 | JWT (jwt-cpp, HS256) + Redis Session |
| 密码加密 | bcrypt |
| 邮件 | mailio (SMTP) |
| 文件存储 | 腾讯云 COS SDK |
| JSON 解析 | picojson / oatpp parser-json |
| 客户端 | Qt 6.8 + QWebSocket |
| 构建 | CMake (服务端) / qmake (客户端) |

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
- 群聊消息 (全员推送)
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

### WebSocket 消息协议 (端口 4567)

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

## 数据库表结构

| 表名 | 说明 | 关键字段 |
|------|------|----------|
| `users` | 用户表 | uuid, username, email, password_hash(bcrypt), status |
| `user_settings` | 用户设置 | theme, language, notification_enabled |
| `friendships` | 好友关系 | user_id, friend_id, status(pending/accepted/blocked/deleted) |
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
- **Linux**: GCC/Clang (C++17), CMake 3.16+, Qt 6.x (客户端)

**服务端依赖 (通过 vcpkg)**：
```bash
vcpkg install oatpp:x64-windows \
    oatpp-websocket:x64-windows \
    oatpp-postgresql:x64-windows \
    oatpp-swagger:x64-windows \
    poco:x64-windows \
    openssl:x64-windows \
    redis++:x64-windows \
    mailio:x64-windows
```

**外部库 (项目内 `lib/` 目录)**：
- bcrypt (预编译 `.lib`)
- 腾讯云 COS SDK (`cos-cpp`)

### 1. 数据库初始化

```bash
# 确保 PostgreSQL 运行中
psql -U postgres -f ChatServer-sql/cmdinit.sql
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

| 字段 | 类型 | 说明 |
|------|------|------|
| `Pagination` | number | 默认分页大小 |
| `server_host` | string | 监听地址, `0.0.0.0` 表示所有网卡 |
| `server_port` | number | HTTP 监听端口 |
| `postgresql_connection_string` | string | PostgreSQL 连接字符串 (libpq 格式) |
| `redis_url` | string | Redis 连接 URL (`tcp://host:port`) |
| `jwt_secret` | string | JWT HS256 签名密钥 (生产环境请更换) |
| `jwt_issuer` | string | JWT 签发者标识 |
| `smtp_host` | string | SMTP 服务器地址 (QQ邮箱: `smtp.qq.com`) |
| `smtp_port` | number | SMTP 端口 (QQ邮箱 SSL: `465`) |
| `sender_name` | string | 发件人显示名称 |
| `sender_email` | string | 发件人邮箱地址 |
| `sender_password` | string | SMTP 授权码 (非邮箱登录密码) |
| `cos_app_id` | string | 腾讯云 COS 应用 ID |
| `cos_secret_id` | string | 腾讯云 API SecretId |
| `cos_secret_key` | string | 腾讯云 API SecretKey |
| `cos_region` | string | COS 存储桶地域 (如 `ap-guangzhou`) |
| `cos_bucket_name` | string | COS 存储桶名称 |

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

> **注意**: 两个服务器的 `jwt_secret` 必须一致，否则 WebSocket 认证会失败。

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

### 5. 启动顺序

```bash
# 1. 启动 PostgreSQL
# 2. 启动 Redis
redis-server

# 3. 启动 HTTP 服务器
cd ChatServer-http/build/bin
./ChatServer.exe

# 4. 启动 WebSocket 服务器
cd ChatServer-websocket/build/bin
./ChatServer.exe

# 5. 启动客户端
cd ChatClient
./ChatClient.exe
```

### 6. 客户端配置

客户端连接地址在 [global.h](ChatClient/global.h) 中定义：

```cpp
static const QString HTTP_BASE_URL = "http://127.0.0.1:8080";
static const QString WEBSOCKET_URL = "ws://127.0.0.1:4567/ws";
static const int MESSAGE_PAGE_SIZE = 50;
```

## 生产部署建议

### TLS 加密

建议通过 nginx 反向代理添加 TLS：

```nginx
# HTTP API
server {
    listen 443 ssl;
    server_name api.example.com;
    ssl_certificate /path/to/cert.pem;
    ssl_certificate_key /path/to/key.pem;

    location / {
        proxy_pass http://127.0.0.1:8080;
    }
}

# WebSocket
server {
    listen 443 ssl;
    server_name ws.example.com;
    ssl_certificate /path/to/cert.pem;
    ssl_certificate_key /path/to/key.pem;

    location /ws {
        proxy_pass http://127.0.0.1:4567;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
    }
}
```

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
├── websocket/
│   └── AppWebSocket.hpp         # WebSocket 连接管理器
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
5. AppAuthInterceptor → 验证 JWT → 校验 Redis session → 提取 userUuid → 传递给 WebSocket
6. WebSocket 连接建立 → 用户状态设为 online
7. WebSocket 断开 → Redis session 删除 → 用户状态设为 offline
```

JWT 过期时间: 3 天 (`EXPIRESIN = 3600 * 24 * 3`, 在 [global.h](ChatServer-http/src/global.h) 中定义)

## License

本项目仅供学习使用。
