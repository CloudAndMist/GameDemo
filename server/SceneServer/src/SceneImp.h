#ifndef _SceneImp_H_
#define _SceneImp_H_

#include "servant/Application.h"
#include "Scene.h"

using namespace std;
using namespace GameDemo;

/**
 * SceneImp implements the SceneServant interface
 * 所有请求共享全局玩家数据（通过 Application）
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
};

#endif
