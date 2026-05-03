#include "SceneImp.h"
#include "SceneServer.h"
#include "servant/Application.h"
#include "Push.h"
#include <chrono>

using namespace std;
using namespace GameDemo;

//////////////////////////////////////////////////////
void SceneImp::initialize()
{
    TLOG_DEBUG("SceneImp::initialize" << endl);

    // 初始化时获取 LobbyServer 代理并缓存，避免每次调用都查询服务地址
    try
    {
        _lobbyPushPrx = Application::getCommunicator()->stringToProxy<Scene2LobbyPushPrx>(
            "GameDemo.LobbyServer.Scene2LobbyPushObj"
        );
        TLOG_INFO("SceneImp: LobbyServer proxy initialized successfully" << endl);
    }
    catch (exception& e)
    {
        TLOG_ERROR("SceneImp: Failed to initialize LobbyServer proxy: " << e.what() << endl);
    }
}

//////////////////////////////////////////////////////
void SceneImp::destroy()
{
    TLOG_DEBUG("SceneImp::destroy" << endl);
}

//////////////////////////////////////////////////////
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

    // 获取场景中其他玩家，填充响应
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

    // 异步通知 LobbyServer 有新玩家进入
    notifyPlayerEnter(req.playerId, req.sceneId, rsp.self);

    TLOG_DEBUG("SceneImp::enterScene success, playerId=" << req.playerId << ", otherPlayers=" << rsp.players.size() << endl);
    return 0;
}

//////////////////////////////////////////////////////
tars::Int32 SceneImp::move(const MoveReq &req, MoveRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("SceneImp::move playerId=" << req.playerId << ", x=" << req.x << ", y=" << req.y << ", z=" << req.z << endl);

    rsp.ret = 0;
    rsp.msg = "success";

    // 更新玩家位置（全局）
    g_app.updatePlayerPosition(req.playerId, req.x, req.y, req.z);

    // 异步通知 LobbyServer 有玩家移动
    auto* player = g_app.getPlayer(req.playerId);
    if (player)
    {
        notifyPlayerMove(req.playerId, player->sceneId, req.x, req.y, req.z);
    }

    return 0;
}

//////////////////////////////////////////////////////
tars::Int32 SceneImp::heartbeat(const HeartBeatReq &req, tars::TarsCurrentPtr _current_)
{
    g_app.updateHeartbeat(req.playerId);
    return 0;
}

//////////////////////////////////////////////////////
tars::Int32 SceneImp::getScenePlayers(tars::Int64 playerId, tars::Int32 sceneId, GetScenePlayersRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("SceneImp::getScenePlayers playerId=" << playerId << ", sceneId=" << sceneId << endl);

    rsp.ret = 0;
    rsp.msg = "success";
    rsp.players.clear();

    auto& globalPlayers = g_app.getGlobalPlayers();
    TLOG_DEBUG("SceneImp::getScenePlayers globalPlayers.size()=" << globalPlayers.size() << endl);

    for (const auto &kv : globalPlayers)
    {
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
        }
    }

    TLOG_DEBUG("SceneImp::getScenePlayers returning players.size()=" << rsp.players.size() << endl);

    return 0;
}

////////////////////////////////////////////////////
tars::Int32 SceneImp::leaveScene(const LeaveSceneReq &req, LeaveSceneRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("SceneImp::leaveScene playerId=" << req.playerId << ", sceneId=" << req.sceneId << endl);

    rsp.ret = 0;
    rsp.msg = "success";

    // 获取玩家信息用于通知
    auto* player = g_app.getPlayer(req.playerId);
    int sceneId = req.sceneId;
    if (player)
    {
        sceneId = player->sceneId;
    }

    // 从全局数据中移除玩家
    g_app.removePlayer(req.playerId);

    // 异步通知 LobbyServer 有玩家离开
    notifyPlayerLeave(req.playerId, sceneId);

    TLOG_DEBUG("SceneImp::leaveScene success, playerId=" << req.playerId << endl);
    return 0;
}

//////////////////////////////////////////////////////
// 私有方法：异步通知 LobbyServer
//////////////////////////////////////////////////////

void SceneImp::notifyPlayerEnter(long playerId, int sceneId, const PlayerInfo& player)
{
    try
    {
        // 使用缓存的代理直接调用，避免每次都查询服务地址
        _lobbyPushPrx->async_onPlayerEnter(NULL, playerId, sceneId, player);
        
        TLOG_DEBUG("SceneImp::notifyPlayerEnter sent, playerId=" << playerId << endl);
    }
    catch (exception& e)
    {
        TLOG_ERROR("SceneImp::notifyPlayerEnter error: " << e.what() << endl);
    }
}

void SceneImp::notifyPlayerMove(long playerId, int sceneId, float x, float y, float z)
{
    try
    {
        _lobbyPushPrx->async_onPlayerMove(NULL, playerId, sceneId, x, y, z);
        
        TLOG_DEBUG("SceneImp::notifyPlayerMove sent, playerId=" << playerId << endl);
    }
    catch (exception& e)
    {
        TLOG_ERROR("SceneImp::notifyPlayerMove error: " << e.what() << endl);
    }
}

void SceneImp::notifyPlayerLeave(long playerId, int sceneId)
{
    try
    {
        _lobbyPushPrx->async_onPlayerLeave(NULL, playerId, sceneId);
        
        TLOG_DEBUG("SceneImp::notifyPlayerLeave sent, playerId=" << playerId << endl);
    }
    catch (exception& e)
    {
        TLOG_ERROR("SceneImp::notifyPlayerLeave error: " << e.what() << endl);
    }
}
