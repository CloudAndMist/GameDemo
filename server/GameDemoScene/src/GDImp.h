#ifndef _GDImp_H_
#define _GDImp_H_

#include "servant/Application.h"
#include "GD.h"
#include <unordered_map>
#include <mutex>
#include <atomic>

using namespace GD;

/**
 * GDImp - GameDemo Service Implementation
 * V0.1: Combined auth and scene service
 */
class GDImp : public GameServant
{
public:
    virtual ~GDImp() {}
    virtual void initialize();
    virtual void destroy();

    virtual tars::Int32 login(const std::string& openId, tars::Int64& sessionId, tars::Int64& playerId, tars::TarsCurrentPtr current);
    virtual tars::Int32 move(tars::Int64 sessionId, tars::Float x, tars::Float y, tars::TarsCurrentPtr current);
    virtual tars::Int32 enterScene(tars::Int64 sessionId, tars::Int64& playerId, PlayerInfo& self, std::vector<PlayerInfo>& players, tars::TarsCurrentPtr current);
    virtual tars::Int32 logout(tars::Int64 sessionId, tars::TarsCurrentPtr current);

private:
    long generateSessionId();

private:
    static std::unordered_map<tars::Int64, std::string> _sessions;  // sessionId -> openId (共享)
    static std::mutex _sessionMutex;
    static std::atomic<tars::Int64> _sessionIdCounter;

    static std::unordered_map<tars::Int64, PlayerInfo> _players;  // playerId -> PlayerInfo (共享)
    static std::mutex _playersMutex;

    static std::unordered_map<std::string, tars::Int64> _openIdToPlayerId;  // openId -> playerId (复用)
    static std::mutex _openIdMutex;
};

#endif
