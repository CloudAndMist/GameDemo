#include "SceneImp.h"
#include "SceneServer.h"
#include "servant/Application.h"
#include <chrono>

using namespace std;
using namespace GameDemo;

////////////////////////////////////////////////////
void SceneImp::initialize()
{
    TLOG_DEBUG("SceneImp::initialize" << endl);
}

////////////////////////////////////////////////////
void SceneImp::destroy()
{
    TLOG_DEBUG("SceneImp::destroy" << endl);
}

////////////////////////////////////////////////////
tars::Int32 SceneImp::enterScene(const EnterSceneReq &req, EnterSceneRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("SceneImp::enterScene playerId=" << req.playerId << ", sceneId=" << req.sceneId << endl);

    rsp.ret = 0;
    rsp.msg = "success";
    rsp.self.playerId = req.playerId;
    rsp.self.sceneId = req.sceneId;
    rsp.self.level = 1;
    rsp.self.x = 0.0f;
    rsp.self.y = 0.0f;
    rsp.self.z = 0.0f;

    // 添加玩家到全局数据
    GlobalPlayerData playerData;
    playerData.playerId = req.playerId;
    playerData.sceneId = req.sceneId;
    playerData.level = 1;
    playerData.x = 0.0f;
    playerData.y = 0.0f;
    playerData.z = 0.0f;
    playerData.lastHeartbeat = chrono::duration_cast<chrono::milliseconds>(
        chrono::system_clock::now().time_since_epoch()).count();
    
    g_app.addPlayer(req.playerId, playerData);

    // 获取场景中其他玩家
    auto& globalPlayers = g_app.getGlobalPlayers();
    for (const auto &kv : globalPlayers)
    {
        if (kv.second.playerId != req.playerId)
        {
            PlayerInfo info;
            info.playerId = kv.second.playerId;
            info.sceneId = kv.second.sceneId;
            info.level = kv.second.level;
            info.x = kv.second.x;
            info.y = kv.second.y;
            info.z = kv.second.z;
            rsp.players.push_back(info);
        }
    }

    TLOG_DEBUG("SceneImp::enterScene success, playerId=" << req.playerId << ", otherPlayers=" << rsp.players.size() << endl);
    return 0;
}

////////////////////////////////////////////////////
tars::Int32 SceneImp::move(const MoveReq &req, MoveRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("SceneImp::move playerId=" << req.playerId << ", x=" << req.x << ", y=" << req.y << ", z=" << req.z << endl);

    rsp.ret = 0;
    rsp.msg = "success";

    // 更新玩家位置（全局）
    g_app.updatePlayerPosition(req.playerId, req.x, req.y, req.z);

    return 0;
}

////////////////////////////////////////////////////
tars::Int32 SceneImp::heartbeat(const HeartBeatReq &req, tars::TarsCurrentPtr _current_)
{
    g_app.updateHeartbeat(req.playerId);
    return 0;
}

////////////////////////////////////////////////////
tars::Int32 SceneImp::getScenePlayers(tars::Int64 playerId, tars::Int32 sceneId, GetScenePlayersRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("SceneImp::getScenePlayers playerId=" << playerId << ", sceneId=" << sceneId << endl);

    rsp.ret = 0;
    rsp.msg = "success";
    rsp.players.clear();  // DEBUG: 先清空

    // DEBUG: 打印全局玩家数量
    auto& globalPlayers = g_app.getGlobalPlayers();
    TLOG_DEBUG("SceneImp::getScenePlayers globalPlayers.size()=" << globalPlayers.size() << endl);
    TLOG_DEBUG("SceneImp::getScenePlayers rsp.players.size() before loop=" << rsp.players.size() << endl);
    
    int pushCount = 0;
    for (const auto &kv : globalPlayers)
    {
        TLOG_DEBUG("SceneImp::getScenePlayers checking playerId=" << kv.first 
                   << ", sceneId=" << kv.second.sceneId << endl);
        
        // 只返回指定场景的玩家
        if (kv.second.sceneId == sceneId)
        {
            PlayerInfo info;
            info.playerId = kv.second.playerId;
            info.sceneId = kv.second.sceneId;
            info.level = kv.second.level;
            info.x = kv.second.x;
            info.y = kv.second.y;
            info.z = kv.second.z;
            rsp.players.push_back(info);
            pushCount++;
            TLOG_DEBUG("SceneImp::getScenePlayers push_back #" << pushCount << ", playerId=" << kv.second.playerId << endl);
        }
    }
    
    TLOG_DEBUG("SceneImp::getScenePlayers returning players.size()=" << rsp.players.size() << endl);

    return 0;
}
