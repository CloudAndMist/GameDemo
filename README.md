# GameDemo - Tars 游戏联机Demo

基于 Tars 框架实现的多人在线游戏 Demo，涵盖登录、角色创建、大厅、场景同步、Push 推送等核心功能。

## 技术栈

| 类别 | 技术 |
|------|------|
| 框架 | Tars C++ |
| 数据库 | MySQL (角色数据)、TcaplusDB (位置数据) |
| 协议 | Tars RPC |
| 构建 | CMake + Docker |

## 分布式服务架构

```
┌──────────────────────────────────────────────────────────────────────────┐
│                              Client (x N)                                 │
│                           登录 / 移动 / 交互                               │
└──────────────────────────────────────────────────────────────────────────┘
                                     ▲
                                     │ RPC / PushCallback
                                     ▼
┌──────────────────────────────────────────────────────────────────────────┐
│                              LobbyServer                                 │         接入层
│                   登录鉴权 │ 选角管理 │ 连接代理 │ Push推送                │
└──────────────────────────────────────────────────────────────────────────┘
         │                                        ▲
         │                             ↓ sync RPC │  ↑ async RPC
         |                                        ▼
         |                       ┌──────────────────────────────────┐
         |  RPC                  │          SceneServer             │                逻辑层
         |                       │    场景逻辑 │ AOI │ 移动同步      │
         |                       └──────────────────────────────────┘
         │                                        │ 
         │                                        │ RPC 定时同步
         ▼                                        ▼
┌───────────────────────────────────────────────────────────────────────┐
│                               DBServer                                |
│      【DBObj】                               【KVObj】                 |
│                                                                       |
│     MySQL:                                   TcaplusDB:               |           存储层
│    • 角色数据                                 • 高频位置存储            |
│    • 会话管理                                 • 玩家数据                |
└───────────────────────────────────────────────────────────────────────┘

```

**通信说明**：
- **Client ↔ LobbyServer**: RPC 请求 + PushCallback 推送
- **LobbyServer ↔ DBServer (GameDBObj)**: RPC 调用，角色数据读写
- **LobbyServer ↔ SceneServer**: RPC 调用，场景创建、玩家进入/离开
- **SceneServer → LobbyServer**: 异步 RPC，Scene 主动调用 Lobby 的 Push 方法推送玩家事件
- **SceneServer ↔ DBServer (GameDBTcaplus)**: 高频数据持久化存储

## 项目目录

```
game-demo/
├── README.md                          # 项目说明
├── game-demo.md                       # Docker 开发工作流
├── server/                            # 服务端源码
│   ├── GameDemoBase/                  # 共享基础库 (Push回调、Notify)
│   │   ├── GameDemoBase.h
│   │   └── Push.h / Push.tars
│   ├── LobbyServer/                   # 大厅服务 (登录、选角、进入场景)
│   │   ├── src/
│   │   │   ├── Lobby.h / Lobby.tars
│   │   │   ├── LobbyImp.cpp           # 核心逻辑
│   │   │   └── LobbyImp.h
│   │   └── build/
│   ├── SceneServer/                   # 场景服务 (移动同步、AOI九宫格)
│   │   ├── src/
│   │   │   ├── Scene.h / Scene.tars
│   │   │   └── SceneImp.cpp
│   │   └── build/
│   └── GameDB/                        # 数据库服务
│       └── src/
├── client/                            # 测试客户端
│   └── TestGameClientV0.4.5/           # V0.4.5 测试客户端
│       └── src/main.cpp               # Tars RPC 调用 + Push 回调
│
└── version_md/                        # 版本迭代文档
    ├── 00_项目设计总览.md
    ├── 00_版本迭代规划.md
    ├── V0.1_踩坑.md
    ├── V0.1_单Scene最小Demo.md
    ├── V0.2_踩坑.md
    ├── V0.2_分布式基础架构搭建.md            # PushCallback
    ├── V0.2.5_数据职责边界重构.md
    ├── V0.3_移动同步增强.md            # AOI
    ├── V0.4_心跳与断线重连.md
    ├── **V0.4.5_修复playerId&roleId混淆.md**  # 一账户一角色
    ├── V0.5_高频数据持久化.md
    └── V0.6_性能压测.md
```

## 参考文献

- **Tars 官方文档**: https://doc.tarsyun.com/
- **Tars GitHub**: https://github.com/TarsCloud/Tars
- **Tars C++ 文档**: https://github.com/TarsCloud/TarsCpp

## 版本进度

| 版本 | 状态 | 说明 |
|------|------|------|
| V0.1 | ✅ | 单Scene最小Demo |
| V0.2 | ✅ | 基础架构搭建 (PushCallback) |
| V0.2.5 | ✅ | 数据职责边界重构 |
| V0.3 | ✅ | 移动同步增强 (AOI九宫格) |
| V0.4 | ✅ | 心跳与断线重连 |
| V0.4.5 | ✅ | **统一 playerId 体系（一账户一角色）** |
| V0.5 | ✅ | 高频数据持久化 (Redis) |
| V0.6 | 📋 | 性能压测 |
