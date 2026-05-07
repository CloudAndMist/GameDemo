#ifndef _LobbyImp_H_
#define _LobbyImp_H_

#include "servant/Application.h"
#include "Lobby.h"
#include "Push.h"
#include "DB.h"

class LobbyServerApp;

// ========== LobbyServant 实现 ==========

class LobbyImp : public GameDemo::LobbyServant
{
public:
    virtual ~LobbyImp(){};

    virtual void initialize(){};

    virtual void destroy(){};

    // 登录 (V0.4.5: 返回 hasCharacter 和 playerInfo)
    virtual tars::Int32 login(const GameDemo::LoginReq& req, GameDemo::LoginRsp& rsp, tars::TarsCurrentPtr _current_);

    // 注册
    virtual tars::Int32 registerAccount(const GameDemo::RegisterReq& req, GameDemo::RegisterRsp& rsp, tars::TarsCurrentPtr _current_);

    // 创建角色 (V0.4.5: 一账户一角色)
    virtual tars::Int32 createCharacter(const GameDemo::CreateCharacterReq& req, GameDemo::CreateCharacterRsp& rsp, tars::TarsCurrentPtr _current_);

    // 心跳
    virtual tars::Int32 heartbeat(const GameDemo::HeartBeatReq& req, tars::TarsCurrentPtr _current_);

    // 进入场景 (V0.4.5: 使用 EnterSceneReq)
    virtual tars::Int32 enterScene(const GameDemo::EnterSceneReq& req, GameDemo::EnterSceneRsp& rsp, tars::TarsCurrentPtr _current_);

    // 移动
    virtual tars::Int32 move(const GameDemo::MoveReq& req, GameDemo::MoveRsp& rsp, tars::TarsCurrentPtr _current_);

    // 离开场景 (V0.4.5: 使用 LeaveSceneReq)
    virtual tars::Int32 leaveScene(const GameDemo::LeaveSceneReq& req, GameDemo::LeaveSceneRsp& rsp, tars::TarsCurrentPtr _current_);

    // 注册推送
    virtual tars::Int32 registerPush(tars::Int64 playerId, tars::TarsCurrentPtr _current_);

    // 登出 (V0.6: 清理场景+Session+推送连接)
    virtual tars::Int32 logout(tars::Int64 playerId, tars::Int64 sessionKey, tars::TarsCurrentPtr _current_);

private:
    GameDemo::DBServantPrx _dbPrx;
    GameDemo::SceneServantPrx _scenePrx;
};

// ========== Scene2LobbyPush 实现 ==========

class Scene2LobbyPushImp : public GameDemo::Scene2LobbyPush
{
public:
    virtual ~Scene2LobbyPushImp(){};

    virtual void initialize(){};

    virtual void destroy(){};

    // 玩家进入场景通知
    virtual tars::Int32 onPlayerEnter(const vector<tars::Int64>& notifyList, tars::Int64 playerId, 
                                       tars::Int32 sceneId, const GameDemo::PlayerBaseInfo& player, 
                                       tars::TarsCurrentPtr _current_);

    // 玩家移动通知
    virtual tars::Int32 onPlayerMove(const vector<tars::Int64>& notifyList, tars::Int64 playerId, 
                                     tars::Int32 sceneId, tars::Float x, tars::Float y, tars::Float z, 
                                     tars::TarsCurrentPtr _current_);

    // 玩家离开场景通知
    virtual tars::Int32 onPlayerLeave(const vector<tars::Int64>& notifyList, tars::Int64 playerId, 
                                       tars::Int32 sceneId, tars::TarsCurrentPtr _current_);

    // 玩家掉线通知
    virtual tars::Int32 onPlayerOffline(const vector<tars::Int64>& notifyList, tars::Int64 playerId, 
                                        tars::Int32 sceneId, tars::TarsCurrentPtr _current_);

    // 玩家重连通知
    virtual tars::Int32 onPlayerOnline(const vector<tars::Int64>& notifyList, tars::Int64 playerId, 
                                       tars::Int32 sceneId, const GameDemo::PlayerBaseInfo& player, 
                                       tars::TarsCurrentPtr _current_);
};

#endif
