#ifndef _LobbyServer_H_
#define _LobbyServer_H_

#include <iostream>
#include <map>
#include <set>
#include <mutex>
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
    // 注册客户端连接 (用于推送)
    void registerClientPush(tars::Int64 playerId, tars::TarsCurrentPtr current);

    // 场景玩家管理
    void addScenePlayer(tars::Int32 sceneId, tars::Int64 playerId);
    void removeScenePlayer(tars::Int32 sceneId, tars::Int64 playerId);

    // 推送回调类型
    typedef std::function<void(tars::TarsCurrentPtr)> PushCallback;

    // 广播给场景内所有玩家 (排除指定玩家)
    void broadcastToScene(tars::Int32 sceneId, tars::Int64 excludePlayerId, const PushCallback& callback);

private:
    std::map<tars::Int64, tars::Int64> _connToPlayer;
    std::map<tars::Int64, tars::Int64> _playerToConn;
    // 推送数据
    std::map<tars::Int64, tars::TarsCurrentPtr> _playerCurrents;
    std::map<tars::Int32, std::set<tars::Int64>> _scenePlayers;
    std::mutex _connMutex;
    std::mutex _pushMutex;
};

extern LobbyServerApp g_app;

#endif
