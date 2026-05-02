#include "LobbyImp.h"
#include "LobbyServer.h"
#include "servant/Application.h"
#include "DB.h"

using namespace std;
using namespace GameDemo;

/////////////////////////////////////////////////////////////////
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
            
            // 绑定连接和玩家 (通过 playerId 查找 connId)
            // connId 在 Tars 框架中可通过其他方式获取
            
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

