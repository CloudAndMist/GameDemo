#ifndef _LobbyServer_H_
#define _LobbyServer_H_

#include "servant/Application.h"
#include "Lobby.h"
#include "HeartbeatThread.h"  // 同目录
#include "Scene.h"

using namespace tars;
using namespace GameDemo;

/**
 * Session 数据结构 - V0.4.5 一账户一角色
 * playerId = accountId，合二为一
 * Session 使用 playerId 作为 key
 */
struct PlayerSession
{
    tars::Int64 playerId;          // 玩家ID = 账号ID
    tars::Int64 sessionKey;         // 会话密钥
    tars::Int64 lastHeartbeat;     // 最后心跳时间戳
    bool isOnline;                 // 是否在线
    tars::Int32 sceneId;           // 当前所在场景ID（0=不在任何场景）
    tars::Int64 offlineTime;        // 掉线时间戳
};

class LobbyServerApp : public Application
{
public:
    virtual ~LobbyServerApp(){};

    virtual void initialize();
    virtual void destroyApp();

    // ========== 推送管理 ==========
    // 注册客户端连接 (用于推送) - 写操作
    void registerClientPush(tars::Int64 playerId, tars::TarsCurrentPtr current);

    // 查找玩家 Current - 读操作，返回找到的 Current 或 nullptr
    tars::TarsCurrentPtr getPlayerCurrent(tars::Int64 playerId);

    // 推送回调类型
    typedef std::function<void(tars::TarsCurrentPtr)> PushCallback;

    // 根据 notifyList 推送 (V0.2.5 核心改动, V0.3 读写锁优化)
    // 使用读写锁支持多线程并行读
    void pushToNotifyList(const vector<tars::Int64>& notifyList, const PushCallback& callback);

    // ========== Session 管理 (V0.4) ==========
    // 创建 Session
    void createSession(tars::Int64 playerId, tars::Int64 sessionKey);

    // 更新心跳
    void updateHeartbeat(tars::Int64 playerId);

    // 验证 Session
    bool validateSession(tars::Int64 playerId, tars::Int64 sessionKey);

    // 获取 Session
    PlayerSession* getSession(tars::Int64 playerId);

    // 更新场景ID
    void updateSceneId(tars::Int64 playerId, tars::Int32 sceneId);

    // 标记玩家离线
    void setPlayerOffline(tars::Int64 playerId);

    // 标记玩家在线（重连恢复）
    void setPlayerOnline(tars::Int64 playerId);

    // 玩家主动离开（彻底清理 Session + 推送连接）
    void removePlayer(tars::Int64 playerId);

    // 获取所有在线玩家ID
    vector<tars::Int64> getOnlinePlayers();

    // ========== 心跳超时检测 (V0.4) ==========
    // 心跳超时检测（供 HeartbeatThread 调用）
    void checkHeartbeatTimeout();

    // 获取 Session 的场景ID（用于心跳超时时通知 SceneServer）
    tars::Int32 getSessionSceneId(tars::Int64 playerId);

private:
    // V0.4: SceneServer 代理（用于通知玩家掉线/重连）
    SceneServantPrx _scenePrx;
    // ========== 定时任务 (V0.4) ==========
    // 心跳超时检测线程 - 使用独立的 HeartbeatThread 类
    HeartbeatThread _heartbeatThread;
    
    // ========== 数据存储 ==========
    // 推送数据: 只持有 playerId -> Current 映射，不持有场景状态
    // V0.3: 使用 unordered_map (O(1)) 替代 map (O(log n))
    std::unordered_map<tars::Int64, tars::TarsCurrentPtr> _playerCurrents;
    
    // V0.3: pthread 读写锁 - 读操作可并行，写操作独占
    mutable pthread_rwlock_t _playerCurrentsRwlock;

    // V0.4: Session 管理
    // playerId -> PlayerSession 映射
    std::unordered_map<tars::Int64, PlayerSession> _sessions;
    mutable pthread_rwlock_t _sessionsRwlock;
};

extern LobbyServerApp g_app;

#endif
