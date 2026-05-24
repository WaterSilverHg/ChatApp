```bash
cd tests
pip install -r requirements.txt

# 运行全部单元测试
pytest test_chatapp.py -v

# 只跑集成测试（跳过慢的压力测试）
pytest test_chatapp.py -v -k "not slow"

# 只跑压力测试
pytest test_chatapp.py -v -k "load"

# 独立压力测试脚本（可调参数）
python load_test.py connect --connections 500    # 500 并发连接
python load_test.py message --senders 50 --messages 200  # 10000 条消息
python load_test.py sustained --duration 120 --connections 30  # 2 分钟持续负载
python load_test.py all                           # 全跑
```