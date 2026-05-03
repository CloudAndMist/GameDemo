#ifndef _SceneImp_H_
#define _SceneImp_H_

#include "servant/Application.h"
#include "Scene.h"
#include "Push.h"

using namespace std;
using namespace GameDemo;

/**
 * SceneImp implements the SceneServant interface
 * 
 * 架构说明:
 * - SceneServer 不管理客户端连接
 * - 通过 Scene2LobbyPush 接口直接调用 LobbyServer
 * - 使用异步 RPC，不阻塞主流程
 */
class SceneImp : public SceneServant
{
public:
    SceneImp(){};
    ~SceneImp(){};

    virtual void initialize() override;
    virtual void destroy() override;

    // SceneServant interface
    virtual tars::Int32 enterScene(const EnterSceneReq &req, EnterSceneRsp &rsp, tars::TarsCurrentPtr _current_) override;
    virtual tars::Int32 move(const MoveReq &req, MoveRsp &rsp, tars::TarsCurrentPtr _current_) override;
    virtual tars::Int32 heartbeat(const HeartBeatReq &req, tars::TarsCurrentPtr _current_) override;
    virtual tars::Int32 getScenePlayers(tars::Int64 playerId, tars::Int32 sceneId, GetScenePlayersRsp &rsp, tars::TarsCurrentPtr _current_) override;
    virtual tars::Int32 leaveScene(const LeaveSceneReq &req, LeaveSceneRsp &rsp, tars::TarsCurrentPtr _current_) override;

private:
    // 缓存 LobbyServer 的推送接口代理
    Scene2LobbyPushPrx _lobbyPushPrx;

    // 通知 LobbyServer 有玩家进入
    void notifyPlayerEnter(long playerId, int sceneId, const PlayerInfo& player);
    
    // 通知 LobbyServer 有玩家移动
    void notifyPlayerMove(long playerId, int sceneId, float x, float y, float z);
    
    // 通知 LobbyServer 有玩家离开
    void notifyPlayerLeave(long playerId, int sceneId);
};

#endif
