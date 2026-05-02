# TARS 协议文件目录

本目录包含 GameDemo 游戏服务器的 TARS 协议定义文件。

## 目录结构

```
tars/                              # 共享协议目录（所有服务共用）
├── GameDemoBase.tars              # 公共基础协议 (枚举、结构体、回调接口)
├── Account.tars                   # 账号相关接口
├── Scene.tars                     # 场景相关接口
├── DB.tars                        # 数据库相关接口
├── Hello.tars                     # HelloServer接口
├── (其他服务.tars)
├── *.h / *.cpp                    # tars2cpp 生成的头文件
└── README.md                      # 本文件
```

## 协议文件说明

### GameDemoBase.tars
- `ErrorCode`: 通用错误码枚举
- `PlayerInfo`: 玩家基础信息结构体
- `SceneInfo`: 场景信息结构体
- `PlayerMoveNotify`: 移动通知结构体
- `PlayerEnterNotify`: 玩家进入通知
- `PlayerLeaveNotify`: 玩家离开通知
- `GameCallback`: 回调接口 (oneway，用于服务端推送)

### Account.tars
- `LobbyServant`: 客户端调用的 Lobby 服务接口
- `AccountServant`: 内部服务调用的账号服务接口
- 登录/注册/创角/选角请求响应结构体

### Scene.tars
- `SceneServant`: 场景服务接口
- `SceneCallback`: 玩家同步回调接口

### DB.tars
- `DBServant`: 数据库服务接口
- 账号/角色数据操作

## 各服务协议文件

每个服务在自己的 `src/` 目录下保存 `.tars` 文件作为源码，
发布时复制到本目录供其他服务引用：

```
server/
├── tars/                          # 共享协议目录（发布目标）
│   ├── GameDemoBase.tars
│   ├── Scene.tars
│   ├── DB.tars
│   └── ...
├── SceneServer/
│   └── src/
│       └── SceneServer.tars       # 服务源码位置
├── DBServer/
│   └── src/
│       └── DBServer.tars          # 服务源码位置
└── HelloServer/
    └── src/
        └── Hello.tars             # 服务源码位置
```

## 生成头文件

在 TARS 环境中执行：

```bash
cd /workspace/server/tars
tars2cpp *.tars
```

这将生成以下文件：
- `GameDemoBase.h/cpp`
- `Account.h/cpp`
- `Scene.h/cpp`
- `DB.h/cpp`
- `Hello.h/cpp`

## 各服务引用方式

在编译各服务时，需要添加 `-I../tars` 包含路径：

```cmake
# 各服务 src/CMakeLists.txt
include_directories(${CMAKE_SOURCE_DIR}/../../tars)
```

## 数据库初始化

```bash
mysql -u root -p < init_db.sql
```
