# TestGameClientV0.2

V0.2 模块测试客户端，用于命令行可视化测试游戏服务。

## 支持测试的功能

| 模块 | 功能 |
|------|------|
| 账号模块 | 注册账号、登录、登出 |
| 角色模块 | 获取角色列表、创建角色、选择角色 |
| 场景模块 | 进入场景、移动、随机移动、离开场景 |
| 心跳模块 | 单次心跳、启动/停止自动心跳 |

## 编译

```bash
# 在 tars-cpp-compiler 容器中
cd /workspace/client/TestGameClientV0.2
mkdir -p build && cd build
cmake ..
make -j4
```

## 使用方法

```bash
# 运行客户端
./TestGameClientV0.2
```

## 命令列表

```
╔══════════════════════════════════════════════════════════════╗
║           TestGameClientV0.2 - V0.2 模块测试客户端          ║
╠══════════════════════════════════════════════════════════════╣
║  [账号模块]                                                   ║
║    1. register <qq> <pwd>  - 注册账号                        ║
║    2. login <qq> <pwd>      - 登录                          ║
║    3. logout                - 登出                          ║
║                                                              ║
║  [角色模块]                                                   ║
║    4. rolelist             - 获取角色列表                    ║
║    5. createrole <name> <job> - 创建角色 (job:1战士/2法师/3猎人) ║
║    6. selectrole <roleId>  - 选择角色                        ║
║                                                              ║
║  [场景模块]                                                   ║
║    7. enter [sceneId]      - 进入场景 (默认sceneId=1)        ║
║    8. move <x> <y> <z>     - 移动                            ║
║    9. randommove           - 随机移动 (后台)                 ║
║   10. leavescene           - 离开场景                       ║
║                                                              ║
║  [心跳模块]                                                   ║
║   11. heartbeat            - 发送一次心跳                    ║
║   12. starthb [sec]       - 启动心跳 (默认10秒间隔)        ║
║   13. stophb               - 停止心跳                        ║
║                                                              ║
║  [系统]                                                       ║
║   14. status               - 查看当前状态                    ║
║   15. help                 - 显示帮助                       ║
║   16. quit                 - 退出程序                        ║
╚══════════════════════════════════════════════════════════════╝
```

## 测试流程示例

```bash
# 1. 注册账号
> register 10001 123456
[OK] 注册成功! accountId=1

# 2. 登录
> login 10001 123456
[OK] 登录成功!
  accountId: 1
  playerId: 0

# 3. 获取角色列表
> rolelist
[OK] 获取成功! 角色数量=0

# 4. 创建角色
> createrole Warrior 1
[OK] 创建成功!
  playerId: 1
  roleId: 1
  roleName: Warrior

# 5. 选择角色
> selectrole 1
[OK] 选择成功!
  playerId: 1
  roleName: Warrior

# 6. 进入场景
> enter 1
[OK] 进入场景成功!
  场景ID: 1
  自身位置: (0, 0, 0)
  场景内其他玩家: 0

# 7. 移动
> move 100.5 200.0 0.0
[OK] 移动成功!

# 8. 随机移动
> randommove
[INFO] 启动随机移动 (后台线程, 3秒间隔)

# 9. 启动心跳
> starthb 10
[INFO] 启动心跳... (间隔 10 秒)
[HEARTBEAT] playerId=1 OK
[HEARTBEAT] playerId=1 OK
...

# 10. 停止心跳
> stophb
[INFO] 停止心跳...

# 11. 离开场景
> leavescene
[OK] 离开场景成功!

# 12. 登出
> logout
[OK] 已登出

# 13. 查看状态
> status
========================================
           当前状态
========================================
  登录状态: 未登录
========================================
```

## 连接配置

- **LobbyServer**: `GameDemo.LobbyServer.LobbyObj`
- **Tars Framework**: `tars-framework:17890`
