#ifndef _LobbyImp_H_
#define _LobbyImp_H_

#include "servant/Application.h"
#include "Lobby.h"
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

private:
    GameDemo::DBServantPrx _dbPrx;
};

#endif
