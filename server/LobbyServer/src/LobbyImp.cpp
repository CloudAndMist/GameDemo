#include "LobbyImp.h"
#include "LobbyServer.h"
#include "servant/Application.h"
#include "DB.h"
#include "Scene.h"

using namespace GameDemo;

extern LobbyServerApp g_app;

tars::Int32 LobbyImp::login(const LoginReq &req, LoginRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("LobbyImp::login qqNumber=" << req.qqNumber << endl);

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
        tars::Int32 ret = _dbPrx->getAccountByQQ(req.qqNumber, account);

        if (ret != 0)
        {
            rsp.ret = ret;
            rsp.msg = "DB error";
            return ret;
        }

        // 验证密码
        if (account.password != req.password)
        {
            rsp.ret = ERR_PASSWORD_MISMATCH;
            rsp.msg = "Password mismatch";
            return rsp.ret;
        }

        // V0.4: 创建 Session
        tars::Int64 sessionKey = TNOW;  // 使用时间戳作为 sessionKey
        g_app.createSession(account.id, sessionKey);

        rsp.ret = 0;
        rsp.msg = "Login success";
        rsp.accountId = account.id;
        rsp.playerId = account.id;
        rsp.qqNumber = account.qqNumber;
        rsp.sessionKey = sessionKey;

        TLOG_DEBUG("Login success, accountId=" << rsp.accountId << ", sessionKey=" << sessionKey << endl);
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

tars::Int32 LobbyImp::registerAccount(const RegisterReq &req, RegisterRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("LobbyImp::registerAccount qqNumber=" << req.qqNumber << endl);

    try
    {
        if (!_dbPrx)
        {
            _dbPrx = Application::getCommunicator()->stringToProxy<GameDemo::DBServantPrx>(
                "GameDemo.DBServer.DBObj"
            );
        }

        // 调用 DBServer 创建账号
        tars::Int32 ret = _dbPrx->createAccount(req.qqNumber, req.password, rsp.accountId);

        if (ret == 0)
        {
            rsp.ret = 0;
            rsp.msg = "Register success";
            TLOG_DEBUG("Register success, accountId=" << rsp.accountId << endl);
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

tars::Int32 LobbyImp::createRole(const CreateRoleReq &req, CreateRoleRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("LobbyImp::createRole accountId=" << req.accountId << ", roleName=" << req.roleName << endl);

    try
    {
        if (!_dbPrx)
        {
            _dbPrx = Application::getCommunicator()->stringToProxy<GameDemo::DBServantPrx>(
                "GameDemo.DBServer.DBObj"
            );
        }

        // 调用 DBServer 创建角色
        tars::Int32 ret = _dbPrx->createRole(req.accountId, req.roleName, req.job, rsp.role);

        if (ret == 0)
        {
            rsp.ret = 0;
            rsp.msg = "Create role success";
            rsp.playerId = rsp.role.id;
            TLOG_DEBUG("Create role success, roleId=" << rsp.role.id << endl);
        }
        else
        {
            rsp.ret = ret;
            rsp.msg = "Create role failed";
            TLOG_ERROR("Create role failed, ret=" << ret << endl);
        }

        return ret;
    }
    catch (exception& e)
    {
        TLOG_ERROR("createRole exception: " << e.what() << endl);
        rsp.ret = ERR_SERVER_BUSY;
        rsp.msg = e.what();
        return rsp.ret;
    }
}

tars::Int32 LobbyImp::getRoleList(const GetRoleListReq &req, GetRoleListRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("LobbyImp::getRoleList accountId=" << req.accountId << endl);

    try
    {
        if (!_dbPrx)
        {
            _dbPrx = Application::getCommunicator()->stringToProxy<GameDemo::DBServantPrx>(
                "GameDemo.DBServer.DBObj"
            );
        }

        // 调用 DBServer 获取角色列表
        tars::Int32 ret = _dbPrx->getRoleList(req.accountId, rsp.roles);

        if (ret == 0)
        {
            rsp.ret = 0;
            rsp.msg = "Get role list success";
            TLOG_DEBUG("Get role list success, count=" << rsp.roles.size() << endl);
        }
        else
        {
            rsp.ret = ret;
            rsp.msg = "Get role list failed";
            TLOG_ERROR("Get role list failed, ret=" << ret << endl);
        }

        return ret;
    }
    catch (exception& e)
    {
        TLOG_ERROR("getRoleList exception: " << e.what() << endl);
        rsp.ret = ERR_SERVER_BUSY;
        rsp.msg = e.what();
        return rsp.ret;
    }
}

tars::Int32 LobbyImp::selectRole(const SelectRoleReq &req, SelectRoleRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("LobbyImp::selectRole accountId=" << req.accountId << ", roleId=" << req.roleId << endl);

    try
    {
        if (!_dbPrx)
        {
            _dbPrx = Application::getCommunicator()->stringToProxy<GameDemo::DBServantPrx>(
                "GameDemo.DBServer.DBObj"
            );
        }

        // 调用 DBServer 获取角色信息
        tars::Int32 ret = _dbPrx->getRole(req.roleId, rsp.role);

        if (ret == 0)
        {
            rsp.ret = 0;
            rsp.msg = "Select role success";
            rsp.playerId = rsp.role.id;

            TLOG_DEBUG("Select role success, playerId=" << rsp.playerId << endl);
        }
        else
        {
            rsp.ret = ret;
            rsp.msg = "Select role failed";
            TLOG_ERROR("Select role failed, ret=" << ret << endl);
        }

        return ret;
    }
    catch (exception& e)
    {
        TLOG_ERROR("selectRole exception: " << e.what() << endl);
        rsp.ret = ERR_SERVER_BUSY;
        rsp.msg = e.what();
        return rsp.ret;
    }
}

tars::Int32 LobbyImp::heartbeat(const HeartBeatReq &req, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("LobbyImp::heartbeat playerId=" << req.playerId << ", sessionKey=" << req.sessionKey << endl);
    
    // V0.4: 验证 sessionKey
    if (!g_app.validateSession(req.playerId, req.sessionKey))
    {
        TLOG_ERROR("LobbyImp::heartbeat: invalid session, playerId=" << req.playerId 
                   << ", sessionKey=" << req.sessionKey << endl);
        return -1;
    }
    
    // V0.4: 更新心跳时间戳
    // 如果玩家是重连（之前离线），会恢复在线状态
    g_app.updateHeartbeat(req.playerId);
    
    return 0;
}

///////////////////////////////////////////////////////////
// 进入场景 (转发给 SceneServer)
tars::Int32 LobbyImp::enterScene(tars::Int64 playerId, tars::Int32 sceneId, EnterSceneRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("LobbyImp::enterScene playerId=" << playerId << ", sceneId=" << sceneId << endl);

    try
    {
        if (!_scenePrx)
        {
            _scenePrx = Application::getCommunicator()->stringToProxy<GameDemo::SceneServantPrx>(
                "GameDemo.SceneServer.SceneObj"
            );
        }

        // V0.4: 验证 Session 存在（Session 在 login 时已创建）
        PlayerSession* session = g_app.getSession(playerId);
        if (!session)
        {
            TLOG_ERROR("LobbyImp::enterScene: session not found, playerId=" << playerId << endl);
            rsp.ret = -1;
            rsp.msg = "session not found";
            return -1;
        }

        // 构建请求
        EnterSceneReq req;
        req.playerId = playerId;
        req.sceneId = sceneId;

        // 调用 SceneServer
        tars::Int32 ret = _scenePrx->enterScene(req, rsp);

        if (ret == 0)
        {
            // V0.4: 更新 session 中的 sceneId
            g_app.updateSceneId(playerId, sceneId);
            TLOG_DEBUG("LobbyImp::enterScene success, playerId=" << playerId << ", sceneId=" << sceneId << endl);
        }
        else
        {
            TLOG_ERROR("LobbyImp::enterScene failed, ret=" << ret << endl);
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

// 移动 (转发给 SceneServer)
tars::Int32 LobbyImp::move(const MoveReq &req, MoveRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("LobbyImp::move playerId=" << req.playerId << endl);

    try
    {
        if (!_scenePrx)
        {
            _scenePrx = Application::getCommunicator()->stringToProxy<GameDemo::SceneServantPrx>(
                "GameDemo.SceneServer.SceneObj"
            );
        }

        // V0.4: 位置信息由 SceneServer 维护，LobbyServer 不再存储

        // 调用 SceneServer
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

// 离开场景 (转发给 SceneServer)
tars::Int32 LobbyImp::leaveScene(tars::Int64 playerId, tars::Int32 sceneId, LeaveSceneRsp &rsp, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("LobbyImp::leaveScene playerId=" << playerId << ", sceneId=" << sceneId << endl);

    try
    {
        if (!_scenePrx)
        {
            _scenePrx = Application::getCommunicator()->stringToProxy<GameDemo::SceneServantPrx>(
                "GameDemo.SceneServer.SceneObj"
            );
        }

        LeaveSceneReq req;
        req.playerId = playerId;
        req.sceneId = sceneId;

        tars::Int32 ret = _scenePrx->leaveScene(req, rsp);

        if (ret == 0)
        {
            // V0.4: 离开场景只是把 sceneId 设为 -1，保留 session
            g_app.updateSceneId(playerId, -1);
            TLOG_DEBUG("LobbyImp::leaveScene success, playerId=" << playerId << ", sceneId=-1 (none)" << endl);
        }
        else
        {
            TLOG_ERROR("LobbyImp::leaveScene failed, ret=" << ret << endl);
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

// 注册推送 (客户端主动调用)
tars::Int32 LobbyImp::registerPush(tars::Int64 playerId, tars::TarsCurrentPtr _current_)
{
    TLOG_INFO("LobbyImp::registerPush playerId=" << playerId << endl);

    // 使用全局 g_app 注册客户端推送
    g_app.registerClientPush(playerId, _current_);

    TLOG_INFO("LobbyImp::registerPush success, playerId=" << playerId << endl);
    return 0;
}

/////////////////////////////////////////////////////////////
///// Scene2LobbyPush 实现 (接收 SceneServer 的回调)
///// V0.2.5: SceneServer 传入 notifyList，LobbyServer 只负责推送
/////////////////////////////////////////////////////////////

// V0.2.5: 根据 notifyList 推送玩家进入通知
tars::Int32 Scene2LobbyPushImp::onPlayerEnter(const vector<tars::Int64>& notifyList, tars::Int64 playerId, tars::Int32 sceneId, const PlayerInfo& player, tars::TarsCurrentPtr _current_)
{
    TLOG_INFO("Scene2LobbyPushImp::onPlayerEnter playerId=" << playerId << ", sceneId=" << sceneId << ", notifyCount=" << notifyList.size() << endl);

    PlayerEnterNotify notify;
    notify.player = player;
    notify.timestamp = TNOW;

    // 根据 SceneServer 传入的 notifyList 推送
    g_app.pushToNotifyList(notifyList, [&notify](tars::TarsCurrentPtr current) {
        Lobby2ClientPush::async_response_push_onPlayerEnter(current, 0, notify);
    });

    return 0;
}

// V0.2.5: 根据 notifyList 推送玩家移动通知
tars::Int32 Scene2LobbyPushImp::onPlayerMove(const vector<tars::Int64>& notifyList, tars::Int64 playerId, tars::Int32 sceneId, tars::Float x, tars::Float y, tars::Float z, tars::TarsCurrentPtr _current_)
{
    TLOG_INFO("Scene2LobbyPushImp::onPlayerMove playerId=" << playerId << ", sceneId=" << sceneId << ", pos=(" << x << "," << y << "," << z << "), notifyCount=" << notifyList.size() << endl);

    PlayerMoveNotify notify;
    notify.playerId = playerId;
    notify.x = x;
    notify.y = y;
    notify.z = z;
    notify.timestamp = TNOW;

    // 根据 SceneServer 传入的 notifyList 推送
    g_app.pushToNotifyList(notifyList, [&notify](tars::TarsCurrentPtr current) {
        Lobby2ClientPush::async_response_push_onPlayerMove(current, 0, notify);
    });

    return 0;
}

// V0.2.5: 根据 notifyList 推送玩家离开通知
tars::Int32 Scene2LobbyPushImp::onPlayerLeave(const vector<tars::Int64>& notifyList, tars::Int64 playerId, tars::Int32 sceneId, tars::TarsCurrentPtr _current_)
{
    TLOG_INFO("Scene2LobbyPushImp::onPlayerLeave playerId=" << playerId << ", sceneId=" << sceneId << ", notifyCount=" << notifyList.size() << endl);

    PlayerLeaveNotify notify;
    notify.playerId = playerId;
    notify.timestamp = TNOW;

    // 根据 SceneServer 传入的 notifyList 推送
    g_app.pushToNotifyList(notifyList, [&notify](tars::TarsCurrentPtr current) {
        Lobby2ClientPush::async_response_push_onPlayerLeave(current, 0, notify);
    });

    return 0;
}

// V0.4: 推送玩家掉线通知
tars::Int32 Scene2LobbyPushImp::onPlayerOffline(const vector<tars::Int64>& notifyList, tars::Int64 playerId, tars::Int32 sceneId, tars::TarsCurrentPtr _current_)
{
    TLOG_INFO("Scene2LobbyPushImp::onPlayerOffline playerId=" << playerId << ", sceneId=" << sceneId << ", notifyCount=" << notifyList.size() << endl);

    // 更新 LobbyServer 内部状态
    g_app.setPlayerOffline(playerId);

    // 推送掉线通知给周围玩家
    PlayerOfflineNotify notify;
    notify.playerId = playerId;
    notify.timestamp = TNOW;

    g_app.pushToNotifyList(notifyList, [&notify](tars::TarsCurrentPtr current) {
        Lobby2ClientPush::async_response_push_onPlayerOffline(current, 0, notify);
    });

    // 推送掉线通知给掉线玩家自己（让其显示掉线状态）
    tars::TarsCurrentPtr selfCurrent = g_app.getPlayerCurrent(playerId);
    if (selfCurrent)
    {
        Lobby2ClientPush::async_response_push_onPlayerOffline(selfCurrent, 0, notify);
    }

    return 0;
}

// V0.4: 推送玩家重连通知
tars::Int32 Scene2LobbyPushImp::onPlayerOnline(const vector<tars::Int64>& notifyList, tars::Int64 playerId, tars::Int32 sceneId, const PlayerInfo& player, tars::TarsCurrentPtr _current_)
{
    TLOG_INFO("Scene2LobbyPushImp::onPlayerOnline playerId=" << playerId << ", sceneId=" << sceneId << ", notifyCount=" << notifyList.size() << endl);

    // 更新 LobbyServer 内部状态
    g_app.setPlayerOnline(playerId);

    // 推送重连通知给周围玩家
    PlayerEnterNotify notify;
    notify.player = player;
    notify.timestamp = TNOW;

    g_app.pushToNotifyList(notifyList, [&notify](tars::TarsCurrentPtr current) {
        Lobby2ClientPush::async_response_push_onPlayerOnline(current, 0, notify);
    });

    // 推送重连通知给重连玩家自己（让其显示在线状态）
    tars::TarsCurrentPtr selfCurrent = g_app.getPlayerCurrent(playerId);
    if (selfCurrent)
    {
        Lobby2ClientPush::async_response_push_onPlayerOnline(selfCurrent, 0, notify);
    }

    return 0;
}
