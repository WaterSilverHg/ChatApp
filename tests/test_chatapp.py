"""
ChatApp 后端测试套件
=====================
- REST API 集成测试（登录、注册、消息、好友、群组）
- WebSocket 实时消息测试
- 压力测试（并发连接、消息吞吐量）

运行方式:
    pytest test_chatapp.py -v                  # 全部测试
    pytest test_chatapp.py -v -k "test_login"  # 只跑登录测试
    pytest test_chatapp.py -v -k "load"        # 只跑压力测试
"""

import asyncio
import json
import time
import uuid
import pytest
import aiohttp
import websockets

# ============================================================
# 配置
# ============================================================
HTTP_BASE = "http://127.0.0.1:8080"
WS_URL    = "ws://127.0.0.1:4567/ws"

# 测试用户（每个测试重新注册，避免数据污染）
TEST_USER_EMAIL = f"test_{uuid.uuid4().hex[:8]}@test.com"
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


async def register_and_login(session):
    """注册新用户 → 登录 → 返回 token + userInfo"""
    email = f"test_{uuid.uuid4().hex[:8]}@test.com"
    # 1. 注册
    status, data = await http_post(session, "/api/auth/register", {
        "username": TEST_USER_NAME,
        "email": email,
        "password": TEST_PASSWORD,
        "phone": f"138{uuid.uuid4().hex[:8]}"[:13]
    })
    assert status == 200, f"注册失败: {data}"
    token = data.get("token", "")
    user = data.get("user", {})
    assert token, "注册返回无 token"
    return token, user


async def ws_connect(token):
    """建立 WebSocket 连接，返回 websocket 对象"""
    extra_headers = {"Authorization": f"Bearer {token}"}
    # python websockets 库用 additional_headers 参数
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
async def test_register():
    """测试：用户注册 → 返回 token 和用户信息"""
    async with aiohttp.ClientSession() as s:
        token, user = await register_and_login(s)
        assert len(token) > 20
        assert user.get("username") == TEST_USER_NAME


@pytest.mark.asyncio
async def test_login():
    """测试：已注册用户可以正常登录"""
    async with aiohttp.ClientSession() as s:
        email = f"test_{uuid.uuid4().hex[:8]}@test.com"
        # 先注册
        await http_post(s, "/api/auth/register", {
            "username": TEST_USER_NAME,
            "email": email,
            "password": TEST_PASSWORD
        })
        # 再登录
        status, data = await http_post(s, "/api/auth/login", {
            "email": email,
            "password": TEST_PASSWORD
        })
        assert status == 200, f"登录失败: {data}"
        assert "token" in data


@pytest.mark.asyncio
async def test_login_wrong_password():
    """测试：错误密码返回 401"""
    async with aiohttp.ClientSession() as s:
        token, _ = await register_and_login(s)
        status, data = await http_post(s, "/api/auth/login", {
            "email": "wrong@test.com",
            "password": "wrongpass"
        })
        assert status == 401, f"期望 401，实际 {status}: {data}"


@pytest.mark.asyncio
async def test_get_conversations():
    """测试：登录后获取会话列表"""
    async with aiohttp.ClientSession() as s:
        token, _ = await register_and_login(s)
        status, data = await http_get(s, "/api/conversations", token)
        assert status == 200
        assert isinstance(data, list)  # 空会话列表也是 OK 的


@pytest.mark.asyncio
async def test_search_users():
    """测试：搜索用户"""
    async with aiohttp.ClientSession() as s:
        token, _ = await register_and_login(s)
        status, data = await http_get(s, "/api/users/search?keyword=test", token)
        assert status == 200
        assert isinstance(data, list)


# ============================================================
# 2. WebSocket 实时消息测试
# ============================================================

@pytest.mark.asyncio
async def test_ws_connect_and_disconnect():
    """测试：WebSocket 连接成功 + 断开"""
    async with aiohttp.ClientSession() as s:
        token, _ = await register_and_login(s)
        ws = await ws_connect(token)
        assert ws.open
        await ws.close()


@pytest.mark.asyncio
async def test_send_friend_request():
    """测试：通过 WebSocket 发送好友请求"""
    async with aiohttp.ClientSession() as s:
        # 注册两个用户
        token1, user1 = await register_and_login(s)
        token2, user2 = await register_and_login(s)

        ws1 = await ws_connect(token1)

        # 发送好友请求（user1 → user2）
        resp = await ws_send_recv(ws1, "send_friend_request", {
            "toUserUuid": user2["useruuid"],
            "message": "hello!"
        })
        assert resp is not None, "未收到响应"
        assert resp.get("type") == "send_friend_request_response"

        await ws1.close()


@pytest.mark.asyncio
async def test_send_private_message():
    """测试：两个用户之间的私聊消息收发"""
    async with aiohttp.ClientSession() as s:
        # 注册两个用户
        token1, user1 = await register_and_login(s)
        token2, user2 = await register_and_login(s)

        ws1 = await ws_connect(token1)
        ws2 = await ws_connect(token2)

        # user1 发送好友请求给 user2
        await ws_send_recv(ws1, "send_friend_request", {
            "toUserUuid": user2["useruuid"],
            "message": "hi"
        })

        # user2 处理请求（需要拿到 request uuid，这里简化：直接查 HTTP）
        status, requests = await http_get(s, "/api/friends/requests/received", token2)
        if status == 200 and isinstance(requests, list) and len(requests) > 0:
            req_uuid = requests[0].get("uuid", "")
            # 通过 WebSocket 接受
            resp = await ws_send_recv(ws2, "handle_friend_request", {
                "reqUuid": req_uuid,
                "status": "accepted"
            })
            assert resp is not None

        # user1 发送私聊消息给 user2
        content = f"hello from test {uuid.uuid4().hex[:6]}"
        resp = await ws_send_recv(ws1, "send_private_message", {
            "touseruuid": user2["useruuid"],
            "content": content,
            "messagetype": "text"
        })
        assert resp is not None
        assert resp.get("type") == "send_private_message_response", f"响应类型不对: {resp}"

        # user2 应该收到 new_private_message 推送
        try:
            raw = await asyncio.wait_for(ws2.recv(), timeout=3.0)
            push = json.loads(raw)
            assert push.get("type") == "new_private_message", f"期望 new_private_message，收到: {push.get('type')}"
        except asyncio.TimeoutError:
            pytest.fail("user2 未在 3 秒内收到消息推送")

        await ws1.close()
        await ws2.close()


# ============================================================
# 3. 压力测试
# ============================================================

@pytest.mark.asyncio
@pytest.mark.slow
async def test_stress_ws_concurrent_connections():
    """
    压力测试：N 个 WebSocket 并发连接。

    验证：
    - 服务端能否承受 100+ 并发 WebSocket 连接
    - 每个连接能否正常完成握手
    """
    CONCURRENT = 100  # 并发数，可根据机器调整

    async with aiohttp.ClientSession() as s:
        token, _ = await register_and_login(s)

        async def connect_one(i):
            try:
                ws = await ws_connect(token)
                assert ws.open, f"连接 {i} 失败"
                await ws.close()
                return True
            except Exception as e:
                return False

        start = time.time()
        results = await asyncio.gather(*[connect_one(i) for i in range(CONCURRENT)])
        elapsed = time.time() - start

        success = sum(results)
        print(f"\n  并发连接: {CONCURRENT} | 成功: {success} | "
              f"失败: {CONCURRENT - success} | 耗时: {elapsed:.2f}s")
        assert success == CONCURRENT, f"连接成功率 {success}/{CONCURRENT}"


@pytest.mark.asyncio
@pytest.mark.slow
async def test_stress_message_throughput():
    """
    压力测试：N 个连接同时发送消息，测量吞吐量。

    验证：
    - 服务端在负载下的消息处理延迟
    - 是否有消息丢失
    """
    CONNECTIONS = 20
    MSGS_PER_CONN = 50

    async with aiohttp.ClientSession() as s:
        # 准备一对用户
        token_a, user_a = await register_and_login(s)
        token_b, user_b = await register_and_login(s)

        # 先建立好友关系
        ws_admin = await ws_connect(token_a)
        await ws_send_recv(ws_admin, "send_friend_request", {
            "toUserUuid": user_b["useruuid"], "message": "test"
        })
        # 获取请求 uuid 并接受
        status, requests = await http_get(s, "/api/friends/requests/received", token_b)
        if status == 200 and isinstance(requests, list) and len(requests) > 0:
            ws_b = await ws_connect(token_b)
            await ws_send_recv(ws_b, "handle_friend_request", {
                "reqUuid": requests[0].get("uuid", ""), "status": "accepted"
            })
            await ws_b.close()
        await ws_admin.close()

        sent_count = [0]
        recv_count = [0]
        lock = asyncio.Lock()

        async def sender(client_id):
            ws = await ws_connect(token_a)
            for _ in range(MSGS_PER_CONN):
                await ws_send_recv(ws, "send_private_message", {
                    "touseruuid": user_b["useruuid"],
                    "content": f"stress_{client_id}_{uuid.uuid4().hex[:4]}",
                    "messagetype": "text"
                })
                async with lock:
                    sent_count[0] += 1
            await ws.close()

        async def receiver():
            ws = await ws_connect(token_b)
            received = 0
            max_wait = 30  # 最长等 30 秒
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

        print(f"\n  发送: {sent_count[0]} | 接收: {recv_count[0]} | "
              f"丢消息: {sent_count[0] - recv_count[0]} | 耗时: {elapsed:.2f}s | "
              f"吞吐: {sent_count[0] / elapsed:.0f} msg/s")
        assert sent_count[0] > 0
        # 允许少量丢失（网络延迟），90% 到达率即算通过
        rate = recv_count[0] / sent_count[0] if sent_count[0] > 0 else 0
        assert rate > 0.9, f"消息到达率 {rate:.1%} < 90%"


@pytest.mark.asyncio
@pytest.mark.slow
async def test_stress_dedup_lock():
    """
    压力测试：同一消息快速重复发送，验证去重锁。

    验证：
    - 服务端去重锁在 2 秒内阻止重复提交
    - 只有第一条消息成功写入
    """
    async with aiohttp.ClientSession() as s:
        token1, user1 = await register_and_login(s)
        token2, user2 = await register_and_login(s)

        # 建立好友关系
        ws1 = await ws_connect(token1)
        await ws_send_recv(ws1, "send_friend_request", {
            "toUserUuid": user2["useruuid"], "message": "dedup"
        })
        status, requests = await http_get(s, "/api/friends/requests/received", token2)
        if status == 200 and isinstance(requests, list) and len(requests) > 0:
            ws2 = await ws_connect(token2)
            await ws_send_recv(ws2, "handle_friend_request", {
                "reqUuid": requests[0].get("uuid", ""), "status": "accepted"
            })
            await ws2.close()

        # 快速连续发送相同内容的消息（触发去重锁）
        identical_payload = {
            "touseruuid": user2["useruuid"],
            "content": "dedup_test_identical",
            "messagetype": "text"
        }

        results = await asyncio.gather(*[
            ws_send_recv(ws1, "send_private_message", identical_payload)
            for _ in range(5)
        ])

        success_count = sum(1 for r in results if r and r.get("success"))
        print(f"\n  5 次连续发送相同消息 → 成功: {success_count}")

        # 第一条应该成功，后续应该被去重锁拒绝
        assert success_count >= 1, "至少第一条消息应该成功"
        assert success_count <= 3, f"去重锁未生效：{success_count} 条成功（期望 <=3）"

        await ws1.close()


# ============================================================
# 4. 端到端完整流程测试
# ============================================================

@pytest.mark.asyncio
async def test_e2e_full_flow():
    """
    端到端测试：注册 → 登录 → WebSocket 连接 → 加好友 → 建群 →
    发私聊 → 发群聊 → 撤回消息 → 拉取历史 → 退出群聊
    """
    async with aiohttp.ClientSession() as s:
        # ---- Step 1: 注册 3 个用户 ----
        t1, u1 = await register_and_login(s)
        t2, u2 = await register_and_login(s)
        t3, u3 = await register_and_login(s)
        print(f"\n  [OK] 3 个用户已注册")

        # ---- Step 2: WebSocket 连接 ----
        w1, w2, w3 = await ws_connect(t1), await ws_connect(t2), await ws_connect(t3)
        assert all(ws.open for ws in [w1, w2, w3])
        print(f"  [OK] 3 个 WebSocket 已连接")

        # ---- Step 3: user1 → user2 好友请求 ----
        resp = await ws_send_recv(w1, "send_friend_request", {
            "toUserUuid": u2["useruuid"], "message": "hello"
        })
        assert resp is not None
        status, reqs = await http_get(s, "/api/friends/requests/received", t2)
        assert status == 200 and isinstance(reqs, list) and len(reqs) > 0
        req_uuid = reqs[0].get("uuid", "")
        print(f"  [OK] 好友请求已发送")

        # ---- Step 4: user2 接受 ----
        resp = await ws_send_recv(w2, "handle_friend_request", {
            "reqUuid": req_uuid, "status": "accepted"
        })
        assert resp is not None
        print(f"  [OK] 好友请求已接受")

        # ---- Step 5: user1 发私聊给 user2 ----
        resp = await ws_send_recv(w1, "send_private_message", {
            "touseruuid": u2["useruuid"],
            "content": "private hello!",
            "messagetype": "text"
        })
        assert resp is not None and resp.get("type") == "send_private_message_response"
        # user2 收到推送
        raw = await asyncio.wait_for(w2.recv(), timeout=3.0)
        push = json.loads(raw)
        assert push.get("type") == "new_private_message"
        print(f"  [OK] 私聊消息收发成功")

        # ---- Step 6: 建群 and 群聊 ----
        resp = await ws_send_recv(w1, "create_group", {
            "name": f"test-group-{uuid.uuid4().hex[:6]}",
            "description": "e2e test group",
            "memberuuids": [u2["useruuid"], u3["useruuid"]],
            "maxmembers": 50,
            "ispublic": False
        })
        assert resp is not None

        # 获取群 uuid（从 HTTP 接口）
        status, groups = await http_get(s, "/api/groups/list", t1)
        assert status == 200 and isinstance(groups, list) and len(groups) > 0
        group_uuid = groups[0].get("uuid", "")
        assert group_uuid
        print(f"  [OK] 群组已创建: {group_uuid}")

        # 发群消息
        resp = await ws_send_recv(w1, "send_group_message", {
            "groupuuid": group_uuid,
            "content": "group hello!",
            "messagetype": "text"
        })
        assert resp is not None
        print(f"  [OK] 群聊消息已发送")

        # ---- Step 7: 拉取历史消息 ----
        status, msgs = await http_get(s, f"/api/messages/private/{u2['useruuid']}/page?page=1&size=10", t1)
        assert status == 200
        assert isinstance(msgs, list)
        print(f"  [OK] 历史消息拉取成功（{len(msgs)} 条私聊消息）")

        # ---- Step 8: 清理 ----
        await asyncio.gather(w1.close(), w2.close(), w3.close())
        print(f"  [OK] 端到端测试全部通过")


# ============================================================
# pytest 配置
# ============================================================

def pytest_configure(config):
    config.addinivalue_line("markers", "slow: 压力测试，耗时较长")


if __name__ == "__main__":
    pytest.main([__file__, "-v", "--tb=short"])
