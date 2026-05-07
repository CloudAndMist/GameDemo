# LoadBotV0.6

压测机器人客户端，用于验证千人在线性能指标。

## 功能

- 批量 Bot 登录（预创建账号，统一场景）
- 初始分散移动 + 随机移动（异步调用，并发窗口控制）
- Push 广播接收与统计
- 心跳保活
- 性能指标自动保存（JSON）

## 构建

```bash
# 在 tars-cpp-compiler 容器中
cd /workspace/client/LoadBotV0.6
mkdir -p build && cd build
cmake .. && make
```

## 运行

```bash
./LoadBotV0_6 <botCount> [moveFreqSec] [sceneId] [durationSec]
```

| 参数 | 说明 | 默认值 | 范围 |
|------|------|--------|------|
| botCount | Bot 数量 | 必填 | 1~1000 |
| moveFreqSec | 每秒移动次数 | 1 | 1~60 |
| sceneId | 场景 ID | 1 | - |
| durationSec | 运行时长(秒)，0=无限 | 60 | - |

示例：

```bash
# 500 Bot, 10次/s, 场景1, 运行60秒
./LoadBotV0_6 500 10 1 60

# 1000 Bot, 5次/s, 场景1, 运行60秒
./LoadBotV0_6 1000 5 1 60
```

## 性能指标

运行时自动采集性能数据，测试结束后写入 JSON 文件到 `metrics/` 目录。

文件命名格式：`{botCount}_bot_{moveFreqSec}_freqSec_{sceneId}_scene_{durationSec}_dur_{timestamp}.json`

指标说明：

| 指标 | 说明 |
|------|------|
| move totalCount | 成功发送的移动请求总数 |
| move avg/p50/p99/max | 移动请求延迟分布 (ms) |
| moveNotify | 收到的移动广播推送总数 |
| avgPerBotPerSec | 每 Bot 每秒收到的推送数 |

## 前置准备

1. 确保 DB 中已预创建 Bot 账号（参见 `init_bot_accounts.sql`）
2. 确保 LobbyServer 和 SceneServer 已启动
3. 在容器内运行，需要 TARS 框架连通

## 设计要点

- **并发窗口**: 每个 Bot 最多 3 个并发 move 请求（MAX_INFLIGHT=3），避免请求洪泛同时充分利用网络管道
- **异常回调**: 处理 `callback_move_exception`，超时/异常时正确释放并发窗口
- **空间分散**: 登录后先随机分散到地图各处，避免扎堆导致广播风暴
