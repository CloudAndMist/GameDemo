// ========== LobbyImp.cpp ==========
// V0.4.5 大厅服务实现
// 一账户一角色：playerId = accountId

#include "LobbyImp.h"
#include "LobbyServer.h"
#include "servant/Application.h"
#include "DB.h"
#include "Scene.h"

using namespace GameDemo;

extern LobbyServerApp g_app;

// ========== 辅助方法 ==========

// 从 AccountInfo 构建 PlayerInfo（不含位置，位置由 SceneServer 管理）
PlayerInfo buildPlayerInfo(const AccountInfo& account)
{
    PlayerInfo info;
    info.playerId = account.id;
    info.playerName = account.playerName;
    info.job = account.job;
    info.level = account.level;
    info.exp = account.exp;
    info.hp = account.hp;
    info.maxHp = account.maxHp;
    info.mp = account.mp;
    info.maxMp = account.maxMp;
    info.createTime = account.createTime;
    info.lastLoginTime = account.lastLoginTime;
    return info;
}

// 从 AccountInfo 构建 PlayerBaseInfo（用于场景推送，不含位置和 isOnline）
PlayerBaseInfo buildPlayerBaseInfo(const AccountInfo& account)
{
    PlayerBaseInfo info;
    info.playerId = account.id;
    info.playerName = account.playerName;
    info.level = account.level;
    // posX, posY, posZ, sceneId 由 SceneServer 在 enterScene 时填充
    return info;
}

// ========== 登录 ==========

tars::Int32 LobbyImp::login(const LoginReq &req, LoginRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("LobbyImp::login username=" << req.username << endl);

    try
    {
        if (!_dbPrx)
        {
            _dbPrx = Application::getCommunicator()->stringToProxy<GameDemo::DBServantPrx>(
                "GameDemo.DBServer.DBObj"
            );
        }

        // 调用 DBServer 获取账号信息
        AccountInfo account;
        tars::Int32 ret = _dbPrx->getAccountByName(req.username, account);

        if (ret != 0)
        {
            rsp.ret = ret;
            rsp.msg = "Account not found";
            return ret;
        }

        // 验证密码
        if (account.password != req.password)
        {
            rsp.ret = ERR_PASSWORD_MISMATCH;
            rsp.msg = "Password mismatch";
            return rsp.ret;
        }

        // V0.4.5: 创建 Session (playerId = accountId)
        tars::Int64 sessionKey = TNOW;
        g_app.createSession(account.id, sessionKey);

        // 更新最后登录时间
        account.lastLoginTime = TNOW;
        _dbPrx->updateAccount(account);

        // 构建响应
        rsp.ret = 0;
        rsp.msg = "Login success";
        rsp.playerId = account.id;           // playerId = accountId
        rsp.sessionKey = sessionKey;

        // V0.4.5: 判断是否已创建角色 (检查 playerName 是否为空)
        rsp.hasCharacter = !account.playerName.empty();

        if (rsp.hasCharacter)
        {
            rsp.playerInfo = buildPlayerInfo(account);
        }

        TLOG_DEBUG("Login success, playerId=" << rsp.playerId 
                   << ", hasCharacter=" << rsp.hasCharacter << endl);
        return 0;
    }
    catch (exception& e)
    {
        TLOG_ERROR("login exception: " << e.what() << endl);
        rsp.ret = ERR_SERVER_BUSY;
        rsp.msg = e.what();
        return rsp.ret;
    }
}

// ========== 注册账号 ==========

tars::Int32 LobbyImp::registerAccount(const RegisterReq &req, RegisterRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("LobbyImp::registerAccount username=" << req.username << endl);

    try
    {
        if (!_dbPrx)
        {
            _dbPrx = Application::getCommunicator()->stringToProxy<GameDemo::DBServantPrx>(
                "GameDemo.DBServer.DBObj"
            );
        }

        // 调用 DBServer 创建账号
        tars::Int32 ret = _dbPrx->createAccount(req.username, req.password, rsp.accountId);

        if (ret == 0)
        {
            rsp.ret = 0;
            rsp.msg = "Register success";
            TLOG_DEBUG("Register success, accountId=" << rsp.accountId << endl);
        }
        else if (ret == ERR_ACCOUNT_EXISTS)
        {
            rsp.ret = ret;
            rsp.msg = "Account already exists";
            TLOG_ERROR("Register failed, account exists" << endl);
        }
        else
        {
            rsp.ret = ret;
            rsp.msg = "Register failed";
            TLOG_ERROR("Register failed, ret=" << ret << endl);
        }

        return ret;
    }
    catch (exception& e)
    {
        TLOG_ERROR("registerAccount exception: " << e.what() << endl);
        rsp.ret = ERR_SERVER_BUSY;
        rsp.msg = e.what();
        return rsp.ret;
    }
}

// ========== 创建角色 ==========

tars::Int32 LobbyImp::createCharacter(const CreateCharacterReq &req, CreateCharacterRsp &rsp, 
                                        tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("LobbyImp::createCharacter playerId=" << req.playerId << ", playerName=" << req.playerName << endl);

    try
    {
        // V0.4.5: 验证 Session
        if (!g_app.validateSession(req.playerId, req.sessionKey))
        {
            rsp.ret = ERR_INVALID_SESSION;
            rsp.msg = "Invalid session";
            TLOG_ERROR("createCharacter: invalid session, playerId=" << req.playerId << endl);
            return rsp.ret;
        }

        if (!_dbPrx)
        {
            _dbPrx = Application::getCommunicator()->stringToProxy<GameDemo::DBServantPrx>(
                "GameDemo.DBServer.DBObj"
            );
        }

        // 检查是否已有角色
        AccountInfo account;
        tars::Int32 ret = _dbPrx->getAccountById(req.playerId, account);
        if (ret != 0)
        {
            rsp.ret = ret;
            rsp.msg = "Account not found";
            return ret;
        }

        if (!account.playerName.empty())
        {
            rsp.ret = ERR_CHARACTER_EXISTS;
            rsp.msg = "Character already exists";
            TLOG_ERROR("createCharacter: character exists, playerId=" << req.playerId << endl);
            return rsp.ret;
        }

        // 初始化角色
        AccountInfo character;
        ret = _dbPrx->initCharacter(req.playerId, req.playerName, req.job, character);

        if (ret == 0)
        {
            rsp.ret = 0;
            rsp.msg = "Character created";
            rsp.playerInfo = buildPlayerInfo(character);
            TLOG_DEBUG("createCharacter success, playerId=" << req.playerId << endl);
        }
        else
        {
            rsp.ret = ret;
            rsp.msg = "Create character failed";
            TLOG_ERROR("createCharacter failed, ret=" << ret << endl);
        }

        return ret;
    }
    catch (exception& e)
    {
        TLOG_ERROR("createCharacter exception: " << e.what() << endl);
        rsp.ret = ERR_SERVER_BUSY;
        rsp.msg = e.what();
        return rsp.ret;
    }
}

// ========== 心跳 ==========

tars::Int32 LobbyImp::heartbeat(const HeartBeatReq &req, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("LobbyImp::heartbeat playerId=" << req.playerId << endl);
    
    // V0.4.5: 验证 sessionKey
    if (!g_app.validateSession(req.playerId, req.sessionKey))
    {
        TLOG_ERROR("LobbyImp::heartbeat: invalid session, playerId=" << req.playerId << endl);
        return ERR_INVALID_SESSION;
    }
    
    // 更新心跳时间戳
    g_app.updateHeartbeat(req.playerId);
    
    return 0;
}

// ========== 进入场景 ==========

tars::Int32 LobbyImp::enterScene(const EnterSceneReq &req, EnterSceneRsp &rsp, 
                                   tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("LobbyImp::enterScene playerId=" << req.playerId << ", sceneId=" << req.sceneId << endl);

    try
    {
        // V0.4.5: 验证 Session
        if (!g_app.validateSession(req.playerId, req.sessionKey))
        {
            rsp.ret = ERR_INVALID_SESSION;
            rsp.msg = "Invalid session";
            TLOG_ERROR("enterScene: invalid session, playerId=" << req.playerId << endl);
            return rsp.ret;
        }

        // 检查是否已创建角色
        if (!_dbPrx)
        {
            _dbPrx = Application::getCommunicator()->stringToProxy<GameDemo::DBServantPrx>(
                "GameDemo.DBServer.DBObj"
            );
        }

        AccountInfo account;
        tars::Int32 ret = _dbPrx->getAccountById(req.playerId, account);
        if (ret != 0 || account.playerName.empty())
        {
            rsp.ret = ERR_CHARACTER_NOT_EXISTS;
            rsp.msg = "Character not found, please create first";
            TLOG_ERROR("enterScene: character not found, playerId=" << req.playerId << endl);
            return rsp.ret;
        }

        if (!_scenePrx)
        {
            _scenePrx = Application::getCommunicator()->stringToProxy<GameDemo::SceneServantPrx>(
                "GameDemo.SceneServer.SceneObj"
            );
        }

        // 转发给 SceneServer
        ret = _scenePrx->enterScene(req, rsp);

        if (ret == 0)
        {
            // 更新 session 中的 sceneId
            g_app.updateSceneId(req.playerId, req.sceneId);
            TLOG_DEBUG("enterScene success, playerId=" << req.playerId << ", sceneId=" << req.sceneId << endl);
        }
        else
        {
            TLOG_ERROR("enterScene failed, ret=" << ret << endl);
        }

        return ret;
    }
    catch (exception& e)
    {
        TLOG_ERROR("enterScene exception: " << e.what() << endl);
        rsp.ret = ERR_SERVER_BUSY;
        rsp.msg = e.what();
        return rsp.ret;
    }
}

// ========== 移动 ==========

tars::Int32 LobbyImp::move(const MoveReq &req, MoveRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("LobbyImp::move playerId=" << req.playerId << endl);

    try
    {
        // V0.4.5: 验证 sessionKey
        if (!g_app.validateSession(req.playerId, req.sessionKey))
        {
            rsp.ret = ERR_INVALID_SESSION;
            rsp.msg = "Invalid session";
            return rsp.ret;
        }

        if (!_scenePrx)
        {
            _scenePrx = Application::getCommunicator()->stringToProxy<GameDemo::SceneServantPrx>(
                "GameDemo.SceneServer.SceneObj"
            );
        }

        // 转发给 SceneServer
        tars::Int32 ret = _scenePrx->move(req, rsp);
        return ret;
    }
    catch (exception& e)
    {
        TLOG_ERROR("move exception: " << e.what() << endl);
        rsp.ret = ERR_SERVER_BUSY;
        rsp.msg = e.what();
        return rsp.ret;
    }
}

// ========== 离开场景 ==========

tars::Int32 LobbyImp::leaveScene(const LeaveSceneReq &req, LeaveSceneRsp &rsp, 
                                    tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("LobbyImp::leaveScene playerId=" << req.playerId << ", sceneId=" << req.sceneId << endl);

    try
    {
        // V0.4.5: 验证 sessionKey
        if (!g_app.validateSession(req.playerId, req.sessionKey))
        {
            rsp.ret = ERR_INVALID_SESSION;
            rsp.msg = "Invalid session";
            return rsp.ret;
        }

        if (!_scenePrx)
        {
            _scenePrx = Application::getCommunicator()->stringToProxy<GameDemo::SceneServantPrx>(
                "GameDemo.SceneServer.SceneObj"
            );
        }

        tars::Int32 ret = _scenePrx->leaveScene(req, rsp);

        if (ret == 0)
        {
            // 离开场景，把 sceneId 设为 0
            g_app.updateSceneId(req.playerId, 0);
            TLOG_DEBUG("leaveScene success, playerId=" << req.playerId << endl);
        }
        else
        {
            TLOG_ERROR("leaveScene failed, ret=" << ret << endl);
        }

        return ret;
    }
    catch (exception& e)
    {
        TLOG_ERROR("leaveScene exception: " << e.what() << endl);
        rsp.ret = ERR_SERVER_BUSY;
        rsp.msg = e.what();
        return rsp.ret;
    }
}

// ========== 注册推送 ==========

tars::Int32 LobbyImp::registerPush(tars::Int64 playerId, tars::TarsCurrentPtr _current_)
{
    TLOG_INFO("LobbyImp::registerPush playerId=" << playerId << endl);

    // 注册客户端推送
    g_app.registerClientPush(playerId, _current_);

    TLOG_INFO("LobbyImp::registerPush success, playerId=" << playerId << endl);
    return 0;
}

// ============================================================
// Scene2LobbyPush 实现 (接收 SceneServer 的回调)
// ============================================================

// 玩家进入通知
tars::Int32 Scene2LobbyPushImp::onPlayerEnter(const vector<tars::Int64>& notifyList, tars::Int64 playerId, 
                                                 tars::Int32 sceneId, const PlayerBaseInfo& player, 
                                                 tars::TarsCurrentPtr _current_)
{
    TLOG_INFO("Scene2LobbyPushImp::onPlayerEnter playerId=" << playerId << ", sceneId=" << sceneId << endl);

    PlayerEnterNotify notify;
    notify.player = player;
    notify.timestamp = TNOW;

    g_app.pushToNotifyList(notifyList, [&notify](tars::TarsCurrentPtr current) {
        Lobby2ClientPush::async_response_push_onPlayerEnter(current, 0, notify);
    });

    return 0;
}

// 玩家移动通知
tars::Int32 Scene2LobbyPushImp::onPlayerMove(const vector<tars::Int64>& notifyList, tars::Int64 playerId, 
                                               tars::Int32 sceneId, tars::Float x, tars::Float y, tars::Float z, 
                                               tars::TarsCurrentPtr _current_)
{
    TLOG_INFO("Scene2LobbyPushImp::onPlayerMove playerId=" << playerId << endl);

    PlayerMoveNotify notify;
    notify.playerId = playerId;
    notify.x = x;
    notify.y = y;
    notify.z = z;
    notify.timestamp = TNOW;

    g_app.pushToNotifyList(notifyList, [&notify](tars::TarsCurrentPtr current) {
        Lobby2ClientPush::async_response_push_onPlayerMove(current, 0, notify);
    });

    return 0;
}

// 玩家离开通知
tars::Int32 Scene2LobbyPushImp::onPlayerLeave(const vector<tars::Int64>& notifyList, tars::Int64 playerId, 
                                                tars::Int32 sceneId, tars::TarsCurrentPtr _current_)
{
    TLOG_INFO("Scene2LobbyPushImp::onPlayerLeave playerId=" << playerId << endl);

    PlayerLeaveNotify notify;
    notify.playerId = playerId;
    notify.timestamp = TNOW;

    g_app.pushToNotifyList(notifyList, [&notify](tars::TarsCurrentPtr current) {
        Lobby2ClientPush::async_response_push_onPlayerLeave(current, 0, notify);
    });

    return 0;
}

// 玩家掉线通知
tars::Int32 Scene2LobbyPushImp::onPlayerOffline(const vector<tars::Int64>& notifyList, tars::Int64 playerId, 
                                                 tars::Int32 sceneId, tars::TarsCurrentPtr _current_)
{
    TLOG_INFO("Scene2LobbyPushImp::onPlayerOffline playerId=" << playerId << endl);

    // 更新 LobbyServer 内部状态
    g_app.setPlayerOffline(playerId);

    // 推送掉线通知给周围玩家
    PlayerOfflineNotify notify;
    notify.playerId = playerId;
    notify.timestamp = TNOW;

    g_app.pushToNotifyList(notifyList, [&notify](tars::TarsCurrentPtr current) {
        Lobby2ClientPush::async_response_push_onPlayerOffline(current, 0, notify);
    });

    // 推送掉线通知给掉线玩家自己
    tars::TarsCurrentPtr selfCurrent = g_app.getPlayerCurrent(playerId);
    if (selfCurrent)
    {
        Lobby2ClientPush::async_response_push_onPlayerOffline(selfCurrent, 0, notify);
    }

    return 0;
}

// 玩家重连通知
tars::Int32 Scene2LobbyPushImp::onPlayerOnline(const vector<tars::Int64>& notifyList, tars::Int64 playerId, 
                                                 tars::Int32 sceneId, const PlayerBaseInfo& player, 
                                                 tars::TarsCurrentPtr _current_)
{
    TLOG_INFO("Scene2LobbyPushImp::onPlayerOnline playerId=" << playerId << endl);

    // 更新 LobbyServer 内部状态
    g_app.setPlayerOnline(playerId);

    // 推送重连通知给周围玩家
    PlayerEnterNotify notify;
    notify.player = player;
    notify.timestamp = TNOW;

    g_app.pushToNotifyList(notifyList, [&notify](tars::TarsCurrentPtr current) {
        Lobby2ClientPush::async_response_push_onPlayerOnline(current, 0, notify);
    });

    // 推送重连通知给重连玩家自己
    tars::TarsCurrentPtr selfCurrent = g_app.getPlayerCurrent(playerId);
    if (selfCurrent)
    {
        Lobby2ClientPush::async_response_push_onPlayerOnline(selfCurrent, 0, notify);
    }

    return 0;
}
