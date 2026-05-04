#include "LobbyImp.h"
#include "LobbyServer.h"
#include "servant/Application.h"
#include "DB.h"
#include "Scene.h"

using namespace std;
using namespace GameDemo;

extern LobbyServerApp g_app;

///////////////////////////////////////////////////////////
tars::Int32 LobbyImp::onConnect(tars::Int64 connId, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("LobbyImp::onConnect connId=" << connId << endl);
    return 0;
}

tars::Int32 LobbyImp::onClose(tars::Int64 connId, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("LobbyImp::onClose connId=" << connId << endl);
    g_app.unbindConnId(connId);
    return 0;
}

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

        rsp.ret = 0;
        rsp.msg = "Login success";
        rsp.accountId = account.id;
        rsp.qqNumber = account.qqNumber;

        TLOG_DEBUG("Login success, accountId=" << rsp.accountId << endl);
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
    TLOG_DEBUG("LobbyImp::heartbeat playerId=" << req.playerId << endl);
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

        // 构建请求
        EnterSceneReq req;
        req.playerId = playerId;
        req.sceneId = sceneId;

        // 调用 SceneServer
        tars::Int32 ret = _scenePrx->enterScene(req, rsp);

        if (ret == 0)
        {
            TLOG_DEBUG("LobbyImp::enterScene success, playerId=" << playerId << endl);
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
            TLOG_DEBUG("LobbyImp::leaveScene success, playerId=" << playerId << endl);
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
