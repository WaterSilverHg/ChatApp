#!/usr/bin/env python3
"""
ChatApp 独立压力测试脚本
=========================
不依赖 pytest，可独立运行。

用法:
    python load_test.py                          # 默认参数
    python load_test.py --connections 500        # 500 并发连接
    python load_test.py --messages 10000         # 发送 10000 条消息
    python load_test.py --connections 200 --ramp 10  # 200 并发，每秒创建 10 个

指标:
    - 连接成功率
    - 消息吞吐量 (msg/s)
    - P50/P95/P99 延迟
"""

import asyncio
import json
import time
import uuid
import argparse
import statistics
import aiohttp
import websockets


async def register_and_login(session: aiohttp.ClientSession, base_url: str):
    """注册并登录，返回 (token, user_info)"""
    email = f"perf_{uuid.uuid4().hex[:8]}@test.com"
    async with session.post(f"{base_url}/api/auth/register", json={
        "username": f"user_{uuid.uuid4().hex[:6]}",
        "email": email,
        "password": "test123456",
    }) as resp:
        data = await resp.json()
    return data.get("token", ""), data.get("user", {})


async def ws_connect(ws_url: str, token: str):
    """建立 WebSocket 连接"""
    return await websockets.connect(
        ws_url,
        additional_headers={"Authorization": f"Bearer {token}"},
        ping_interval=10,
        ping_timeout=12,
        close_timeout=5,
    )


async def connection_benchmark(base_url: str, ws_url: str,
                               num_connections: int, ramp_rate: int):
    """
    连接压力测试: N 个 WebSocket 并发连接

    ramp_rate: 每秒创建多少个连接（控制压力递增速度）
    """
    async with aiohttp.ClientSession() as s:
        # 注册一个共享用户
        token, _ = await register_and_login(s, base_url)

        success = 0
        failures = 0
        times = []

        sem = asyncio.Semaphore(ramp_rate)

        async def connect_one(i):
            nonlocal success, failures
            async with sem:
                t0 = time.monotonic()
                try:
                    ws = await ws_connect(ws_url, token)
                    elapsed = time.monotonic() - t0
                    times.append(elapsed)
                    success += 1
                    await ws.close()
                except Exception:
                    failures += 1
                if (i + 1) % 50 == 0:
                    print(f"  ... {i + 1}/{num_connections}")

        start = time.time()
        await asyncio.gather(*[connect_one(i) for i in range(num_connections)])
        elapsed = time.time() - start

    print(f"\n{'='*60}")
    print(f"  WebSocket 连接压力测试")
    print(f"{'='*60}")
    print(f"  总连接数:     {num_connections}")
    print(f"  成功:         {success}")
    print(f"  失败:         {failures}")
    print(f"  成功率:       {success/num_connections*100:.1f}%")
    print(f"  总耗时:       {elapsed:.2f}s")
    print(f"  创建速率:     {num_connections/elapsed:.0f} conn/s")
    if times:
        print(f"  握手延迟 P50: {statistics.median(times)*1000:.0f}ms")
        print(f"  握手延迟 P95: {statistics.quantiles(times, n=20)[-2]*1000:.0f}ms")
        print(f"  握手延迟 P99: {sorted(times)[int(len(times)*0.99)]*1000:.0f}ms")
    print(f"{'='*60}\n")


async def message_throughput_benchmark(base_url: str, ws_url: str,
                                       num_senders: int, msgs_per_sender: int):
    """
    消息吞吐量测试: N 个发送者各发 M 条消息

    通过一对用户收发来测量服务端处理速率
    """
    async with aiohttp.ClientSession() as s:
        # 注册 2 个用户
        t_a, u_a = await register_and_login(s, base_url)
        t_b, u_b = await register_and_login(s, base_url)

        # 加好友
        w_a = await ws_connect(ws_url, t_a)
        msg = {"type": "send_friend_request",
               "toUserUuid": u_b["useruuid"], "message": "perf"}
        await w_a.send(json.dumps(msg))
        await w_a.recv()
        await w_a.close()

        # 获取请求 uuid 并接受
        headers = {"Authorization": f"Bearer {t_b}"}
        async with s.get(f"{base_url}/api/friends/requests/received",
                          headers=headers) as resp:
            reqs = await resp.json()
        req_uuid = reqs[0]["uuid"]

        w_b = await ws_connect(ws_url, t_b)
        msg = {"type": "handle_friend_request",
               "reqUuid": req_uuid, "status": "accepted"}
        await w_b.send(json.dumps(msg))
        await w_b.recv()
        await w_b.close()

        # ---- 开始测吞吐 ----
        send_times = []
        recv_count = 0
        recv_lock = asyncio.Lock()

        async def sender(client_id):
            ws = await ws_connect(ws_url, t_a)
            for seq in range(msgs_per_sender):
                payload = {"type": "send_private_message",
                           "touseruuid": u_b["useruuid"],
                           "content": f"perf_{client_id}_{seq}",
                           "messagetype": "text"}
                t0 = time.monotonic()
                await ws.send(json.dumps(payload))
                await ws.recv()  # 等待响应
                send_times.append(time.monotonic() - t0)
            await ws.close()

        async def receiver():
            nonlocal recv_count
            ws = await ws_connect(ws_url, t_b)
            total = num_senders * msgs_per_sender
            received = 0
            t0 = time.time()

            while received < total:
                try:
                    raw = await asyncio.wait_for(ws.recv(), timeout=5.0)
                    msg = json.loads(raw)
                    if msg.get("type") == "new_private_message":
                        recv_count += 1
                        received += 1
                except asyncio.TimeoutError:
                    if time.time() - t0 > 60:
                        print(f"  接收超时: {received}/{total}")
                        break
            await ws.close()

        print(f"  开始发送 {num_senders}x{msgs_per_sender} = "
              f"{num_senders * msgs_per_sender} 条消息...")

        start = time.time()
        recv_task = asyncio.create_task(receiver())
        await asyncio.gather(*[sender(i) for i in range(num_senders)])
        await recv_task
        elapsed = time.time() - start

    print(f"\n{'='*60}")
    print(f"  消息吞吐量测试")
    print(f"{'='*60}")
    print(f"  发送者数:     {num_senders}")
    print(f"  每人发送:     {msgs_per_sender}")
    print(f"  总发送:       {num_senders * msgs_per_sender}")
    print(f"  总接收:       {recv_count}")
    print(f"  丢失:         {num_senders * msgs_per_sender - recv_count}")
    print(f"  耗时:         {elapsed:.2f}s")
    print(f"  吞吐量:       {num_senders * msgs_per_sender / elapsed:.0f} msg/s")
    if send_times:
        print(f"  发送延迟 P50: {statistics.median(send_times)*1000:.0f}ms")
        print(f"  发送延迟 P95: {statistics.quantiles(send_times, n=20)[-2]*1000:.0f}ms")
        print(f"  发送延迟 P99: {sorted(send_times)[int(len(send_times)*0.99)]*1000:.0f}ms")
    print(f"{'='*60}\n")


async def sustained_load_test(base_url: str, ws_url: str, duration_sec: int,
                              connections: int, msg_interval_ms: int):
    """
    持续负载测试: 维持 N 个长连接，每个每隔 interval_ms 发一条消息，持续 duration_sec

    模拟真实聊天场景的长期负载
    """
    async with aiohttp.ClientSession() as s:
        t_a, u_a = await register_and_login(s, base_url)
        t_b, u_b = await register_and_login(s, base_url)

        # 加好友
        w = await ws_connect(ws_url, t_a)
        await w.send(json.dumps({"type": "send_friend_request",
                                  "toUserUuid": u_b["useruuid"], "message": "load"}))
        await w.recv()
        await w.close()

        headers = {"Authorization": f"Bearer {t_b}"}
        async with s.get(f"{base_url}/api/friends/requests/received",
                          headers=headers) as resp:
            reqs = await resp.json()
        req_uuid = reqs[0]["uuid"]

        w = await ws_connect(ws_url, t_b)
        await w.send(json.dumps({"type": "handle_friend_request",
                                  "reqUuid": req_uuid, "status": "accepted"}))
        await w.recv()
        await w.close()

        total_sent = 0
        total_errs = 0
        stop_flag = asyncio.Event()

        async def worker(worker_id):
            nonlocal total_sent, total_errs
            ws = await ws_connect(ws_url, t_a)
            seq = 0
            while not stop_flag.is_set():
                try:
                    payload = {"type": "send_private_message",
                               "touseruuid": u_b["useruuid"],
                               "content": f"sustained_{worker_id}_{seq}",
                               "messagetype": "text"}
                    await ws.send(json.dumps(payload))
                    await asyncio.wait_for(ws.recv(), timeout=3.0)
                    total_sent += 1
                    seq += 1
                    await asyncio.sleep(msg_interval_ms / 1000.0)
                except Exception:
                    total_errs += 1
            await ws.close()

        # 启动所有 worker
        tasks = [asyncio.create_task(worker(i)) for i in range(connections)]

        print(f"  持续负载运行中... ({duration_sec}s, {connections} 连接, "
              f"间隔 {msg_interval_ms}ms)")
        for i in range(duration_sec):
            await asyncio.sleep(1)
            if (i + 1) % 10 == 0:
                print(f"  ... {i + 1}s | 已发送 {total_sent} | 错误 {total_errs}")

        stop_flag.set()
        await asyncio.gather(*tasks, return_exceptions=True)

    print(f"\n{'='*60}")
    print(f"  持续负载测试 ({duration_sec}s)")
    print(f"{'='*60}")
    print(f"  并发连接:     {connections}")
    print(f"  发送间隔:     {msg_interval_ms}ms")
    print(f"  总发送:       {total_sent}")
    print(f"  总错误:       {total_errs}")
    print(f"  平均吞吐:     {total_sent / duration_sec:.0f} msg/s")
    print(f"  单连接吞吐:   {total_sent / duration_sec / connections:.1f} msg/s/conn")
    print(f"{'='*60}\n")


# ============================================================
# CLI
# ============================================================

def main():
    parser = argparse.ArgumentParser(description="ChatApp 压力测试")
    parser.add_argument("--http-url", default="http://127.0.0.1:8080")
    parser.add_argument("--ws-url", default="ws://127.0.0.1:4567/ws")

    sub = parser.add_subparsers(dest="cmd")

    # connection benchmark
    p = sub.add_parser("connect", help="连接压力测试")
    p.add_argument("--connections", type=int, default=100)
    p.add_argument("--ramp", type=int, default=20, help="每秒创建连接数")

    # message benchmark
    p = sub.add_parser("message", help="消息吞吐量测试")
    p.add_argument("--senders", type=int, default=10)
    p.add_argument("--messages", type=int, default=100)

    # sustained load
    p = sub.add_parser("sustained", help="持续负载测试")
    p.add_argument("--duration", type=int, default=60)
    p.add_argument("--connections", type=int, default=20)
    p.add_argument("--interval", type=int, default=500,
                   help="每条消息间隔(ms)")

    # all
    p = sub.add_parser("all", help="运行全部压力测试")

    args = parser.parse_args()

    async def run():
        if args.cmd == "connect" or args.cmd == "all":
            await connection_benchmark(args.http_url, args.ws_url,
                                       args.connections, args.ramp)
        if args.cmd == "message" or args.cmd == "all":
            senders = getattr(args, 'senders', 10)
            msgs = getattr(args, 'messages', 100)
            await message_throughput_benchmark(args.http_url, args.ws_url,
                                               senders, msgs)
        if args.cmd == "sustained" or args.cmd == "all":
            await sustained_load_test(args.http_url, args.ws_url,
                                      args.duration, args.connections,
                                      args.interval)

    asyncio.run(run())


if __name__ == "__main__":
    # 默认：运行连接 + 消息测试
    import sys
    if len(sys.argv) == 1:
        sys.argv.extend(["all", "--connections", "100",
                         "--messages", "50", "--duration", "30"])

    main()
