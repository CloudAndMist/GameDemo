#ifndef _SceneImp_H_
#define _SceneImp_H_

#include "servant/Application.h"
#include "Scene.h"
#include "Push.h"
#include "PlayerManager.h"

using namespace std;
using namespace GameDemo;

/**
 * SceneImp implements the SceneServant interface
 * 
 * 架构说明:
 * - SceneServer 不管理客户端连接
 * - 通过 Scene2LobbyPush 接口直接调用 LobbyServer
 * - 使用异步 RPC，不阻塞主流程
 * - 玩家数据由 SceneServer 全局单例 PlayerManager 统一管理
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
    virtual tars::Int32 leaveScene(const LeaveSceneReq &req, LeaveSceneRsp &rsp, tars::TarsCurrentPtr _current_) override;
    virtual tars::Int32 playerOffline(tars::Int64 playerId, tars::Int32 sceneId, tars::TarsCurrentPtr _current_) override;
    virtual tars::Int32 playerOnline(tars::Int64 playerId, tars::Int32 sceneId, tars::TarsCurrentPtr _current_) override;

private:
    // 缓存常用组件指针（由 SceneServer 统一管理生命周期）
    PlayerManager* _playerMgr;        // 指向 SceneServer 中的全局 PlayerManager
    Scene2LobbyPushPrx _lobbyPushPrx;  // LobbyServer 推送接口代理

    // 通知 LobbyServer 有玩家进入（notifyList 由调用方计算并传入）
    void notifyPlayerEnter(tars::Int64 playerId, tars::Int32 sceneId, const PlayerInfo& player, const vector<tars::Int64>& notifyList);

    // 通知 LobbyServer 有玩家移动（notifyList 由调用方计算并传入）
    void notifyPlayerMove(tars::Int64 playerId, tars::Int32 sceneId, float x, float y, float z, const vector<tars::Int64>& notifyList);

    // 通知 LobbyServer 有玩家离开（notifyList 由调用方计算并传入）
    void notifyPlayerLeave(tars::Int64 playerId, tars::Int32 sceneId, const vector<tars::Int64>& notifyList);

    // 通知 LobbyServer 有玩家掉线（notifyList 由调用方计算并传入）
    void notifyPlayerOffline(tars::Int64 playerId, tars::Int32 sceneId, const vector<tars::Int64>& notifyList);

    // 通知 LobbyServer 有玩家重连（notifyList 由调用方计算并传入）
    void notifyPlayerOnline(tars::Int64 playerId, tars::Int32 sceneId, const PlayerInfo& player, const vector<tars::Int64>& notifyList);
};

#endif
