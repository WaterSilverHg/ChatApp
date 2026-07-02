"""
ChatApp 后端测试套件
=====================
- REST API 集成测试（登录、会话、好友、群组、文件）
- WebSocket 实时消息测试
- 压力测试（并发连接、消息吞吐量）

运行方式:
    pytest test_chatapp.py -v                  # 全部测试
    pytest test_chatapp.py -v -k "login"       # 只跑登录测试
    pytest test_chatapp.py -v -k "not slow"    # 排除压力测试
"""

import asyncio
import json
import time
import uuid
import os
import pytest
import aiohttp
import websockets

# ============================================================
# 配置
# ============================================================
HTTP_BASE = "http://127.0.0.1:8080"
WS_URL    = "ws://127.0.0.1:4567/ws"

TEST_USER_EMAIL = "test@test.com"
TEST_USER_NAME  = "testuser"
TEST_PASSWORD   = "12345678"


# ============================================================
# 辅助工具
# ============================================================

async def http_post(session, path, data, token=None):
    """发送带认证的 POST 请求"""
    headers = {"Content-Type": "application/json"}
    if token:
        headers["Authorization"] = f"Bearer {token}"
    async with session.post(f"{HTTP_BASE}{path}", json=data, headers=headers) as resp:
        return resp.status, await resp.json()


async def http_get(session, path, token):
    """发送带认证的 GET 请求"""
    headers = {"Authorization": f"Bearer {token}"}
    async with session.get(f"{HTTP_BASE}{path}", headers=headers) as resp:
        return resp.status, await resp.json()


async def http_put(session, path, data, token):
    headers = {"Content-Type": "application/json",
               "Authorization": f"Bearer {token}"}
    async with session.put(f"{HTTP_BASE}{path}", json=data, headers=headers) as resp:
        return resp.status, await resp.json()


async def login(session, email=None, password=None):
    """登录已有用户 → 返回 token + userInfo"""
    if email is None:
        email = TEST_USER_EMAIL
    if password is None:
        password = TEST_PASSWORD
    
    status, data = await http_post(session, "/api/auth/login", {
        "email": email,
        "password": password
    })
    assert status == 200, f"登录失败: {data}"
    token = data.get("token", "")
    user = data.get("user", {})
    assert token, "登录返回无 token"
    return token, user


async def ws_connect(token):
    """建立 WebSocket 连接，返回 websocket 对象"""
    extra_headers = {"Authorization": f"Bearer {token}"}
    try:
        ws = await websockets.connect(
            WS_URL,
            additional_headers=extra_headers,
            ping_interval=10,
            ping_timeout=12,
            close_timeout=5
        )
        return ws
    except Exception as e:
        pytest.fail(f"WebSocket 连接失败: {e}")


async def ws_send_recv(ws, msg_type, data):
    """发送 WebSocket 消息并等待响应"""
    msg = {"type": msg_type, **data}
    await ws.send(json.dumps(msg))
    try:
        raw = await asyncio.wait_for(ws.recv(), timeout=5.0)
        return json.loads(raw)
    except asyncio.TimeoutError:
        return None


# ============================================================
# 1. REST API 集成测试
# ============================================================

@pytest.mark.asyncio
async def test_login():
    """测试：已注册用户可以正常登录"""
    async with aiohttp.ClientSession() as s:
        status, data = await http_post(s, "/api/auth/login", {
            "email": TEST_USER_EMAIL,
            "password": TEST_PASSWORD
        })
        assert status == 200, f"登录失败: {data}"
        assert "token" in data
        assert "user" in data


@pytest.mark.asyncio
async def test_login_wrong_password():
    """测试：错误密码返回 401"""
    async with aiohttp.ClientSession() as s:
        status, data = await http_post(s, "/api/auth/login", {
            "email": "wrong@test.com",
            "password": "wrongpass"
        })
        assert status == 401, f"期望 401，实际 {status}: {data}"


@pytest.mark.asyncio
async def test_get_conversations():
    """测试：登录后获取会话列表"""
    async with aiohttp.ClientSession() as s:
        token, _ = await login(s)
        status, data = await http_get(s, "/api/conversations", token)
        assert status == 200
        assert isinstance(data, list)


@pytest.mark.asyncio
async def test_search_users():
    """测试：搜索用户"""
    async with aiohttp.ClientSession() as s:
        token, _ = await login(s)
        status, data = await http_get(s, "/api/users/search?keyword=test", token)
        assert status == 200
        assert isinstance(data, list)


@pytest.mark.asyncio
async def test_friends_list():
    """测试：获取好友列表"""
    async with aiohttp.ClientSession() as s:
        token, _ = await login(s)
        status, data = await http_get(s, "/api/friends/list", token)
        assert status == 200
        assert isinstance(data, list)


@pytest.mark.asyncio
async def test_groups_list():
    """测试：获取群组列表"""
    async with aiohttp.ClientSession() as s:
        token, _ = await login(s)
        status, data = await http_get(s, "/api/groups/list", token)
        assert status == 200
        assert isinstance(data, list)


@pytest.mark.asyncio
async def test_update_status():
    """测试：更新用户状态"""
    async with aiohttp.ClientSession() as s:
        token, _ = await login(s)
        status, data = await http_put(s, "/api/status", {"status": "online"}, token)
        assert status == 200


@pytest.mark.asyncio
async def test_file_upload():
    """测试：上传文件（使用 tests/test.jpg）"""
    test_file = os.path.join(os.path.dirname(__file__), "test.jpg")
    
    async with aiohttp.ClientSession() as s:
        token, _ = await login(s)
        
        status, data = await http_post(s, "/api/files/record", {
            "fileName": "test.jpg",
            "fileSize": os.path.getsize(test_file),
            "fileType": "image/jpeg"
        }, token)
        assert status == 200, f"创建记录失败: {data}"
        file_uuid = data.get("uuid", "")
        assert file_uuid, "未返回 fileUuid"
        
        with open(test_file, "rb") as f:
            file_data = f.read()
        
        headers = {
            "Authorization": f"Bearer {token}",
            "fileUuid": file_uuid,
            "Content-Type": "application/octet-stream"
        }
        async with s.post(f"{HTTP_BASE}/api/files/upload", data=file_data, headers=headers) as resp:
            status = resp.status
            data = await resp.json()
            assert status == 200, f"上传失败: {data}"


# ============================================================
# 2. WebSocket 实时消息测试
# ============================================================

@pytest.mark.asyncio
async def test_ws_connect_and_disconnect():
    """测试：WebSocket 连接成功 + 断开"""
    async with aiohttp.ClientSession() as s:
        token, _ = await login(s)
        ws = await ws_connect(token)
        assert ws.open
        await ws.close()


@pytest.mark.asyncio
async def test_send_friend_request():
    """测试：通过 WebSocket 发送好友请求"""
    async with aiohttp.ClientSession() as s:
        token1, user1 = await login(s)
        token2, user2 = await login(s, "test2@test.com", "12345678")

        ws1 = await ws_connect(token1)

        resp = await ws_send_recv(ws1, "send_friend_request", {
            "toUserUuid": user2["userUuid"],
            "message": "hello!"
        })
        assert resp is not None, "未收到响应"

        await ws1.close()


@pytest.mark.asyncio
async def test_send_private_message():
    """测试：两个用户之间的私聊消息收发"""
    async with aiohttp.ClientSession() as s:
        token1, user1 = await login(s)
        token2, user2 = await login(s, "test2@test.com", "12345678")

        ws1 = await ws_connect(token1)
        ws2 = await ws_connect(token2)

        content = f"hello from test {uuid.uuid4().hex[:6]}"
        resp = await ws_send_recv(ws1, "send_private_message", {
            "touseruuid": user2["userUuid"],
            "content": content,
            "messagetype": "text"
        })
        assert resp is not None

        await ws1.close()
        await ws2.close()


# ============================================================
# 3. 压力测试
# ============================================================

@pytest.mark.asyncio
@pytest.mark.slow
async def test_stress_ws_concurrent_connections():
    """压力测试：并发 WebSocket 连接"""
    CONCURRENT = 50

    async with aiohttp.ClientSession() as s:
        token, _ = await login(s)

        async def connect_one(i):
            try:
                ws = await ws_connect(token)
                assert ws.open
                await ws.close()
                return True
            except Exception as e:
                return False

        start = time.time()
        results = await asyncio.gather(*[connect_one(i) for i in range(CONCURRENT)])
        elapsed = time.time() - start

        success = sum(results)
        print(f"\n  并发连接: {CONCURRENT} | 成功: {success} | 耗时: {elapsed:.2f}s")
        assert success == CONCURRENT, f"连接成功率 {success}/{CONCURRENT}"


@pytest.mark.asyncio
@pytest.mark.slow
async def test_stress_message_throughput():
    """压力测试：消息吞吐量"""
    CONNECTIONS = 10
    MSGS_PER_CONN = 20

    async with aiohttp.ClientSession() as s:
        token_a, user_a = await login(s)
        token_b, user_b = await login(s, "test2@test.com", "12345678")

        sent_count = [0]
        recv_count = [0]
        lock = asyncio.Lock()

        async def sender(client_id):
            ws = await ws_connect(token_a)
            for _ in range(MSGS_PER_CONN):
                await ws_send_recv(ws, "send_private_message", {
                    "touseruuid": user_b["userUuid"],
                    "content": f"stress_{client_id}_{uuid.uuid4().hex[:4]}",
                    "messagetype": "text"
                })
                async with lock:
                    sent_count[0] += 1
            await ws.close()

        async def receiver():
            ws = await ws_connect(token_b)
            received = 0
            max_wait = 30
            t0 = time.time()
            while received < CONNECTIONS * MSGS_PER_CONN:
                try:
                    raw = await asyncio.wait_for(ws.recv(), timeout=2.0)
                    msg = json.loads(raw)
                    if msg.get("type") == "new_private_message":
                        recv_count[0] += 1
                        received += 1
                except asyncio.TimeoutError:
                    if time.time() - t0 > max_wait:
                        break
            await ws.close()

        start = time.time()
        recv_task = asyncio.create_task(receiver())
        await asyncio.gather(*[sender(i) for i in range(CONNECTIONS)])
        await recv_task
        elapsed = time.time() - start

        print(f"\n  发送: {sent_count[0]} | 接收: {recv_count[0]} | 耗时: {elapsed:.2f}s")
        assert sent_count[0] > 0


# ============================================================
# pytest 配置
# ============================================================

def pytest_configure(config):
    config.addinivalue_line("markers", "slow: 压力测试，耗时较长")


if __name__ == "__main__":
    pytest.main([__file__, "-v", "--tb=short"])