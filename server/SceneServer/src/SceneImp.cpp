#include "SceneImp.h"
#include "SceneServer.h"
#include "servant/Application.h"
#include "Push.h"

using namespace std;
using namespace GameDemo;

//////////////////////////////////////////////////
void SceneImp::initialize()
{
    TLOG_DEBUG("SceneImp::initialize" << endl);

    // 缓存 PlayerManager 指针（初始化顺序保证：SceneServer::initialize() 先执行）
    _playerMgr = &g_app.getPlayerManager();

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

//////////////////////////////////////////////////
void SceneImp::destroy()
{
    TLOG_DEBUG("SceneImp::destroy" << endl);
}

//////////////////////////////////////////////////
tars::Int32 SceneImp::enterScene(const EnterSceneReq &req, EnterSceneRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("SceneImp::enterScene playerId=" << req.playerId << ", sceneId=" << req.sceneId << endl);

    rsp.ret = 0;
    rsp.msg = "success";

    // 获取进入前的旧视野玩家（用于跨场景切换时通知）
    vector<tars::Int64> oldViewPlayers = _playerMgr->getViewPlayers(req.playerId);
    int32_t oldSceneId = 0;
    auto* existingPlayer = _playerMgr->getPlayer(req.playerId);
    if (existingPlayer) {
        oldSceneId = existingPlayer->sceneId;
    }

    // V0.5: 从 Redis 恢复玩家数据（内存无 → 恢复/初始化）
    // 返回 true 表示发生了跨场景切换
    bool crossSceneSwitch = _playerMgr->restoreOnEnter(req.playerId, req.sceneId);

    // 获取当前玩家信息
    auto* selfPlayer = _playerMgr->getPlayer(req.playerId);
    if (selfPlayer) {
        rsp.self = _playerMgr->toPlayerBaseInfo(*selfPlayer);
    } else {
        // 不太可能发生，但做保护
        rsp.self.playerId = req.playerId;
        rsp.self.sceneId = req.sceneId;
        rsp.self.level = 1;
        rsp.self.posX = 0.0f;
        rsp.self.posY = 0.0f;
        rsp.self.posZ = 0.0f;
    }

    // 获取视野内玩家（基于新场景）
    vector<tars::Int64> viewPlayers = _playerMgr->getViewPlayers(req.playerId);

    // 填充视野内其他玩家信息
    for (tars::Int64 pid : viewPlayers)
    {
        auto* p = _playerMgr->getPlayer(pid);
        if (p)
        {
            rsp.players.push_back(_playerMgr->toPlayerBaseInfo(*p));
        }
    }

    // 跨场景切换：先通知旧场景玩家该玩家已离开
    if (crossSceneSwitch && oldSceneId != req.sceneId && !oldViewPlayers.empty()) {
        notifyPlayerLeave(req.playerId, oldSceneId, oldViewPlayers);
        TLOG_DEBUG("SceneImp::enterScene notified " << oldViewPlayers.size() 
                  << " players in old scene " << oldSceneId << endl);
    }

    // 异步通知 LobbyServer 有新玩家进入
    notifyPlayerEnter(req.playerId, req.sceneId, rsp.self, viewPlayers);

    TLOG_DEBUG("SceneImp::enterScene success, playerId=" << req.playerId << ", otherPlayers=" << rsp.players.size() << endl);
    return 0;
}

//////////////////////////////////////////////////
tars::Int32 SceneImp::move(const MoveReq &req, MoveRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("SceneImp::move playerId=" << req.playerId << ", x=" << req.x << ", y=" << req.y << ", z=" << req.z << endl);

    rsp.ret = 0;
    rsp.msg = "success";

    // 更新玩家位置（一次性完成数据和 AOI 格子更新）
    _playerMgr->playerMove(req.playerId, req.x, req.y, req.z);

    // 获取视野内玩家
    vector<tars::Int64> viewPlayers = _playerMgr->getViewPlayers(req.playerId);

    // 异步通知 LobbyServer 有玩家移动
    auto* player = _playerMgr->getPlayer(req.playerId);
    if (player)
    {
        notifyPlayerMove(req.playerId, player->sceneId, req.x, req.y, req.z, viewPlayers);
    }

    return 0;
}

//////////////////////////////////////////////////
tars::Int32 SceneImp::leaveScene(const LeaveSceneReq &req, LeaveSceneRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("SceneImp::leaveScene playerId=" << req.playerId << ", sceneId=" << req.sceneId << endl);

    rsp.ret = 0;
    rsp.msg = "success";

    // 获取玩家信息用于通知
    auto* player = _playerMgr->getPlayer(req.playerId);
    tars::Int32 sceneId = req.sceneId;
    if (player)
    {
        sceneId = player->sceneId;
    }

    // 获取离开前需要通知的玩家（此时玩家还在 AOI 中）
    vector<tars::Int64> notifyList = _playerMgr->getViewPlayers(req.playerId);

    // 从玩家管理器移除（一次性完成 AOI 格子移除和数据移除）
    _playerMgr->playerLeave(req.playerId);

    // 异步通知 LobbyServer 有玩家离开
    notifyPlayerLeave(req.playerId, sceneId, notifyList);

    TLOG_DEBUG("SceneImp::leaveScene success, playerId=" << req.playerId << endl);
    return 0;
}

//////////////////////////////////////////////////
// 私有方法：异步通知 LobbyServer
//////////////////////////////////////////////////

void SceneImp::notifyPlayerEnter(tars::Int64 playerId, tars::Int32 sceneId, const PlayerBaseInfo& player, const vector<tars::Int64>& notifyList)
{
    try
    {
        _lobbyPushPrx->async_onPlayerEnter(NULL, notifyList, playerId, sceneId, player);

        TLOG_DEBUG("SceneImp::notifyPlayerEnter sent, playerId=" << playerId << ", notifyList size=" << notifyList.size() << endl);
    }
    catch (exception& e)
    {
        TLOG_ERROR("SceneImp::notifyPlayerEnter error: " << e.what() << endl);
    }
}

void SceneImp::notifyPlayerMove(tars::Int64 playerId, tars::Int32 sceneId, float x, float y, float z, const vector<tars::Int64>& notifyList)
{
    try
    {
        _lobbyPushPrx->async_onPlayerMove(NULL, notifyList, playerId, sceneId, x, y, z);

        TLOG_DEBUG("SceneImp::notifyPlayerMove sent, playerId=" << playerId << ", notifyList size=" << notifyList.size() << endl);
    }
    catch (exception& e)
    {
        TLOG_ERROR("SceneImp::notifyPlayerMove error: " << e.what() << endl);
    }
}

void SceneImp::notifyPlayerLeave(tars::Int64 playerId, tars::Int32 sceneId, const vector<tars::Int64>& notifyList)
{
    try
    {
        _lobbyPushPrx->async_onPlayerLeave(NULL, notifyList, playerId, sceneId);

        TLOG_DEBUG("SceneImp::notifyPlayerLeave sent, playerId=" << playerId << ", notifyList size=" << notifyList.size() << endl);
    }
    catch (exception& e)
    {
        TLOG_ERROR("SceneImp::notifyPlayerLeave error: " << e.what() << endl);
    }
}

void SceneImp::notifyPlayerOffline(tars::Int64 playerId, tars::Int32 sceneId, const vector<tars::Int64>& notifyList)
{
    try
    {
        _lobbyPushPrx->async_onPlayerOffline(NULL, notifyList, playerId, sceneId);

        TLOG_DEBUG("SceneImp::notifyPlayerOffline sent, playerId=" << playerId << ", notifyList size=" << notifyList.size() << endl);
    }
    catch (exception& e)
    {
        TLOG_ERROR("SceneImp::notifyPlayerOffline error: " << e.what() << endl);
    }
}

void SceneImp::notifyPlayerOnline(tars::Int64 playerId, tars::Int32 sceneId, const PlayerBaseInfo& player, const vector<tars::Int64>& notifyList)
{
    try
    {
        _lobbyPushPrx->async_onPlayerOnline(NULL, notifyList, playerId, sceneId, player);

        TLOG_DEBUG("SceneImp::notifyPlayerOnline sent, playerId=" << playerId << ", notifyList size=" << notifyList.size() << endl);
    }
    catch (exception& e)
    {
        TLOG_ERROR("SceneImp::notifyPlayerOnline error: " << e.what() << endl);
    }
}

//============================================================
tars::Int32 SceneImp::playerOffline(tars::Int64 playerId, tars::Int32 sceneId, tars::TarsCurrentPtr _current_)
{
    TLOG_INFO("SceneImp::playerOffline playerId=" << playerId
              << ", sceneId=" << sceneId << endl);

    // 获取玩家并检查状态
    auto* player = _playerMgr->getPlayer(playerId);
    if (!player) {
        TLOG_WARN("playerOffline: player not found, playerId=" << playerId << endl);
        return -1;
    }

    // V0.4: 检查玩家是否已经是离线状态，避免重复通知
    if (!player->isOnline) {
        TLOG_INFO("playerOffline: player already offline, skip, playerId=" << playerId << endl);
        return 0;
    }

    // V0.5: 断线必须同步保存数据到 Redis
    _playerMgr->onPlayerOffline(playerId);

    // 通知周围玩家该玩家掉线
    vector<tars::Int64> notifyList = _playerMgr->getViewPlayers(playerId);
    if (!notifyList.empty()) {
        notifyPlayerOffline(playerId, sceneId, notifyList);
    }

    TLOG_INFO("SceneImp::playerOffline success, playerId=" << playerId << endl);
    return 0;
}

tars::Int32 SceneImp::playerOnline(tars::Int64 playerId, tars::Int32 sceneId, tars::TarsCurrentPtr _current_)
{
    TLOG_INFO("SceneImp::playerOnline playerId=" << playerId
              << ", sceneId=" << sceneId << endl);

    // 获取玩家并检查状态
    auto* player = _playerMgr->getPlayer(playerId);
    if (!player) {
        TLOG_WARN("playerOnline: player not found, playerId=" << playerId << endl);
        return -1;
    }

    // V0.4: 检查玩家是否已经在线，避免重复通知
    if (player->isOnline) {
        TLOG_INFO("playerOnline: player already online, skip, playerId=" << playerId << endl);
        return 0;
    }

    // 标记为在线（不改变 AOI 结构，只改变状态标志）
    _playerMgr->setOnline(playerId, true);

    // 通知周围玩家该玩家重连
    vector<tars::Int64> notifyList = _playerMgr->getViewPlayers(playerId);
    if (!notifyList.empty()) {
        PlayerBaseInfo playerInfo;
        playerInfo.playerId = player->playerId;
        playerInfo.sceneId = player->sceneId;
        playerInfo.level = player->level;
        playerInfo.posX = player->x;
        playerInfo.posY = player->y;
        playerInfo.posZ = player->z;
        notifyPlayerOnline(playerId, sceneId, playerInfo, notifyList);
    }

    TLOG_INFO("SceneImp::playerOnline success, playerId=" << playerId << endl);
    return 0;
}
