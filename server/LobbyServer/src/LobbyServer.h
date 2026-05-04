#ifndef _LobbyServer_H_
#define _LobbyServer_H_

#include <iostream>
#include <unordered_map>
#include <pthread.h>
#include <functional>
#include "servant/Application.h"
#include "Lobby.h"

using namespace tars;
using namespace GameDemo;

class LobbyServerApp : public Application
{
public:
    virtual ~LobbyServerApp(){};

    virtual void initialize();
    virtual void destroyApp();

    // connId -> playerId 映射管理
    void bindConnId(tars::Int64 connId, tars::Int64 playerId);
    void unbindConnId(tars::Int64 connId);
    tars::Int64 getPlayerIdByConnId(tars::Int64 connId);
    tars::Int64 getConnIdByPlayerId(tars::Int64 playerId);

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

private:
    std::map<tars::Int64, tars::Int64> _connToPlayer;
    std::map<tars::Int64, tars::Int64> _playerToConn;
    
    // 推送数据: 只持有 playerId -> Current 映射，不持有场景状态
    // V0.3: 使用 unordered_map (O(1)) 替代 map (O(log n))
    std::unordered_map<tars::Int64, tars::TarsCurrentPtr> _playerCurrents;
    
    // V0.3: pthread 读写锁 - 读操作可并行，写操作独占
    mutable pthread_rwlock_t _playerCurrentsRwlock;
    
    std::mutex _connMutex;
};

extern LobbyServerApp g_app;

#endif
