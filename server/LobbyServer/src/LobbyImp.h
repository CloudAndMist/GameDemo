#ifndef _LobbyImp_H_
#define _LobbyImp_H_

#include "servant/Application.h"
#include "Lobby.h"
#include "Push.h"
#include "DB.h"

class LobbyServerApp;

class LobbyImp : public GameDemo::LobbyServant
{
public:
    virtual ~LobbyImp(){};

    virtual void initialize(){};

    virtual void destroy(){};

    // 处理客户端连接
    virtual tars::Int32 onConnect(tars::Int64 connId, tars::TarsCurrentPtr _current_);

    // 处理客户端断开
    virtual tars::Int32 onClose(tars::Int64 connId, tars::TarsCurrentPtr _current_);

    // 登录
    virtual tars::Int32 login(const GameDemo::LoginReq& req, GameDemo::LoginRsp& rsp, tars::TarsCurrentPtr _current_);

    // 注册
    virtual tars::Int32 registerAccount(const GameDemo::RegisterReq& req, GameDemo::RegisterRsp& rsp, tars::TarsCurrentPtr _current_);

    // 创建角色
    virtual tars::Int32 createRole(const GameDemo::CreateRoleReq& req, GameDemo::CreateRoleRsp& rsp, tars::TarsCurrentPtr _current_);

    // 获取角色列表
    virtual tars::Int32 getRoleList(const GameDemo::GetRoleListReq& req, GameDemo::GetRoleListRsp& rsp, tars::TarsCurrentPtr _current_);

    // 选择角色
    virtual tars::Int32 selectRole(const GameDemo::SelectRoleReq& req, GameDemo::SelectRoleRsp& rsp, tars::TarsCurrentPtr _current_);

    // 心跳
    virtual tars::Int32 heartbeat(const GameDemo::HeartBeatReq& req, tars::TarsCurrentPtr _current_);

    // 进入场景 (转发给 SceneServer)
    virtual tars::Int32 enterScene(tars::Int64 playerId, tars::Int32 sceneId, GameDemo::EnterSceneRsp& rsp, tars::TarsCurrentPtr _current_);

    // 移动 (转发给 SceneServer)
    virtual tars::Int32 move(const GameDemo::MoveReq& req, GameDemo::MoveRsp& rsp, tars::TarsCurrentPtr _current_);

    // 离开场景 (转发给 SceneServer)
    virtual tars::Int32 leaveScene(tars::Int64 playerId, tars::Int32 sceneId, GameDemo::LeaveSceneRsp& rsp, tars::TarsCurrentPtr _current_);

    // 注册推送 (客户端主动调用以接收推送)
    virtual tars::Int32 registerPush(tars::Int64 playerId, tars::TarsCurrentPtr _current_);

private:
    GameDemo::DBServantPrx _dbPrx;
    GameDemo::SceneServantPrx _scenePrx;
};

// Scene2LobbyPush 实现 (供 SceneServer 调用)
// 根据 V0.2.5: SceneServer 传入 notifyList，LobbyServer 只负责根据列表推送，不持有场景状态
class Scene2LobbyPushImp : public GameDemo::Scene2LobbyPush
{
public:
    virtual ~Scene2LobbyPushImp(){};

    virtual void initialize(){};

    virtual void destroy(){};

    // 玩家进入场景通知
    // notifyList: Scene 内除新玩家外的所有 playerId，由 SceneServer 计算并传入
    virtual tars::Int32 onPlayerEnter(const vector<tars::Int64>& notifyList, tars::Int64 playerId, tars::Int32 sceneId, const GameDemo::PlayerInfo& player, tars::TarsCurrentPtr _current_);

    // 玩家移动通知
    // notifyList: Scene 内其他玩家的 playerId，由 SceneServer 计算并传入
    virtual tars::Int32 onPlayerMove(const vector<tars::Int64>& notifyList, tars::Int64 playerId, tars::Int32 sceneId, tars::Float x, tars::Float y, tars::Float z, tars::TarsCurrentPtr _current_);

    // 玩家离开场景通知
    // notifyList: Scene 内其他玩家的 playerId，由 SceneServer 计算并传入
    virtual tars::Int32 onPlayerLeave(const vector<tars::Int64>& notifyList, tars::Int64 playerId, tars::Int32 sceneId, tars::TarsCurrentPtr _current_);
};

#endif
