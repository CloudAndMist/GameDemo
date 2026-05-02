#ifndef _LobbyServer_H_
#define _LobbyServer_H_

#include <iostream>
#include <map>
#include <mutex>
#include "servant/Application.h"
#include "Lobby.h"

using namespace tars;
using namespace GameDemo;

class LobbyServerApp : public Application
{
public:
    virtual ~LobbyServerApp(){};

    virtual void initialize();
    virtual void destroyApp();

    // connId -> playerId 映射管理
    void bindConnId(tars::Int64 connId, tars::Int64 playerId);
    void unbindConnId(tars::Int64 connId);
    tars::Int64 getPlayerIdByConnId(tars::Int64 connId);
    tars::Int64 getConnIdByPlayerId(tars::Int64 playerId);

private:
    std::map<tars::Int64, tars::Int64> _connToPlayer;
    std::map<tars::Int64, tars::Int64> _playerToConn;
    std::mutex _mutex;
};

extern LobbyServerApp g_app;

#endif
