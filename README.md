# GameDemo - Tars 游戏联机Demo

基于 Tars 框架实现的多人在线游戏 Demo，涵盖登录、角色创建、大厅、场景同步、Push 推送等核心功能。

## 技术栈

| 类别 | 技术 |
|------|------|
| 框架 | Tars C++ |
| 数据库 | MySQL (角色数据)、TcaplusDB (位置数据) |
| 协议 | Tars RPC |
| 构建 | CMake + Docker |

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
│   └── TestGameClientV0.2/
│       └── src/main.cpp               # Tars RPC 调用 + Push 回调
│
└── version_md/                        # 版本迭代文档
    ├── 00_项目设计总览.md
    ├── V0.1_单Scene最小Demo.md
    ├── V0.2_分布式基础架构搭建.md            # PushCallback
    ├── V0.3_移动同步增强.md            # AOI
    ├── V0.5_性能压测.md
    └── V0.6_高频数据持久化(TODO).md
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
| V0.3 | ⏳ | 移动同步增强 (AOI九宫格) |
| V0.5 | ⏳ | 性能压测 |
| V0.6 | 📋 | 高频数据持久化 (TODO) |
