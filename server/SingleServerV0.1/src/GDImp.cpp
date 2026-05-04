#include "GDImp.h"
#include "util/tc_common.h"
#include <chrono>

using namespace std;
using namespace GD;

// 静态成员定义（所有 GDImp 实例共享）
std::unordered_map<tars::Int64, std::string> GDImp::_sessions;
std::mutex GDImp::_sessionMutex;
std::atomic<tars::Int64> GDImp::_sessionIdCounter{1000};

std::unordered_map<tars::Int64, PlayerInfo> GDImp::_players;
std::mutex GDImp::_playersMutex;

std::unordered_map<std::string, tars::Int64> GDImp::_openIdToPlayerId;
std::mutex GDImp::_openIdMutex;

//////////////////////////////////////////////////////

void GDImp::initialize()
{
    TLOGDEBUG("GDImp::initialize()" << endl);
}

//////////////////////////////////////////////////////

void GDImp::destroy()
{
    TLOGDEBUG("GDImp::destroy(), players: " << _players.size() << endl);
}

//////////////////////////////////////////////////////

tars::Int32 GDImp::login(const std::string& openId, tars::Int64& sessionId, tars::Int64& playerId, tars::TarsCurrentPtr current)
{
    TLOGDEBUG("GDImp::login, openId: " << openId << endl);

    if (openId.empty() || openId.length() < 3)
    {
        return -1;
    }

    sessionId = generateSessionId();

    // 复用已有 playerId（基于 openId），保持玩家数据一致
    {
        lock_guard<mutex> lock(_openIdMutex);
        auto it = _openIdToPlayerId.find(openId);
        if (it != _openIdToPlayerId.end())
        {
            playerId = it->second;
            TLOGDEBUG("GDImp::login reused playerId: " << playerId << " for openId: " << openId << endl);
        }
        else
        {
            playerId = sessionId;
            _openIdToPlayerId[openId] = playerId;
            TLOGDEBUG("GDImp::login created new playerId: " << playerId << " for openId: " << openId << endl);
        }
    }

    {
        lock_guard<mutex> lock(_sessionMutex);
        _sessions[sessionId] = openId;
    }

    TLOGDEBUG("GDImp::login success, sessionId: " << sessionId << ", playerId: " << playerId << endl);

    return 0;
}

//////////////////////////////////////////////////////

tars::Int32 GDImp::move(tars::Int64 sessionId, tars::Float x, tars::Float y, tars::TarsCurrentPtr current)
{
    TLOGDEBUG("GDImp::move, sessionId: " << sessionId
              << ", pos: (" << x << ", " << y << ")" << endl);

    // Validate session and get openId
    std::string openId;
    {
        lock_guard<mutex> lock(_sessionMutex);
        auto it = _sessions.find(sessionId);
        if (it == _sessions.end())
        {
            return -1;
        }
        openId = it->second;
    }

    // 获取复用后的 playerId
    long playerId;
    {
        lock_guard<mutex> lock(_openIdMutex);
        auto it = _openIdToPlayerId.find(openId);
        if (it != _openIdToPlayerId.end())
        {
            playerId = it->second;
        }
        else
        {
            return -1;
        }
    }

    {
        lock_guard<mutex> lock(_playersMutex);
        auto it = _players.find(playerId);
        if (it == _players.end())
        {
            return -1;
        }

        it->second.x = x;
        it->second.y = y;
    }

    return 0;
}

//////////////////////////////////////////////////////

tars::Int32 GDImp::enterScene(tars::Int64 sessionId, tars::Int64& playerId, PlayerInfo& self, std::vector<PlayerInfo>& players, tars::TarsCurrentPtr current)
{
    TLOGDEBUG("GDImp::enterScene, sessionId: " << sessionId << endl);

    // Validate session and get openId
    std::string openId;
    {
        lock_guard<mutex> lock(_sessionMutex);
        auto it = _sessions.find(sessionId);
        if (it == _sessions.end())
        {
            return -1;
        }
        openId = it->second;
    }

    // 通过 openId 获取复用后的 playerId
    {
        lock_guard<mutex> lock(_openIdMutex);
        auto it = _openIdToPlayerId.find(openId);
        if (it != _openIdToPlayerId.end())
        {
            playerId = it->second;
        }
        else
        {
            // 不应该发生，login 时应该已经创建
            playerId = sessionId;
        }
    }

    // 检查是否已有玩家数据（复用位置）
    bool isReturning = false;
    {
        lock_guard<mutex> lock(_playersMutex);
        auto it = _players.find(playerId);
        if (it != _players.end())
        {
            // 复用已有数据
            self = it->second;
            isReturning = true;
            TLOGDEBUG("GDImp::enterScene reused existing player, playerId: " << playerId 
                      << ", pos: (" << self.x << ", " << self.y << ")" << endl);
        }
        else
        {
            // 新玩家
            self.playerId = playerId;
            self.openId = openId;
            self.x = 0.0f;
            self.y = 0.0f;
            self.enterTime = TC_Common::now2ms() / 1000;
            TLOGDEBUG("GDImp::enterScene new player, playerId: " << playerId << endl);
        }
    }
    
    TLOGDEBUG("GDImp::enterScene returning self, playerId: " << self.playerId 
              << ", pos: (" << self.x << ", " << self.y << ")" << endl);

    // 更新进入时间
    self.enterTime = TC_Common::now2ms() / 1000;

    // Get other players
    {
        lock_guard<mutex> lock(_playersMutex);

        for (auto& kv : _players)
        {
            if (kv.first != playerId)
            {
                players.push_back(kv.second);
            }
        }

        _players[playerId] = self;
    }

    TLOGDEBUG("GDImp::enterScene success, playerId: " << playerId << ", others: " << players.size() << endl);

    return 0;
}

//////////////////////////////////////////////////////

tars::Int32 GDImp::logout(tars::Int64 sessionId, tars::TarsCurrentPtr current)
{
    // 获取 playerId（不直接从 sessionId 获取，因为 sessionId 可能会变化）
    std::string openId;
    long playerId = -1;
    
    {
        lock_guard<mutex> lock(_sessionMutex);
        auto it = _sessions.find(sessionId);
        if (it != _sessions.end())
        {
            openId = it->second;
        }
        else
        {
            return -1;
        }
    }
    
    // 通过 openId 获取 playerId
    {
        lock_guard<mutex> lock(_openIdMutex);
        auto it = _openIdToPlayerId.find(openId);
        if (it != _openIdToPlayerId.end())
        {
            playerId = it->second;
        }
    }
    
    TLOGDEBUG("GDImp::logout, sessionId: " << sessionId << ", playerId: " << playerId << endl);

    // 只删除 session，保留玩家数据（位置等）以便下次进入时复用
    {
        lock_guard<mutex> lock(_sessionMutex);
        _sessions.erase(sessionId);
    }

    return 0;
}

//////////////////////////////////////////////////////

long GDImp::generateSessionId()
{
    return ++_sessionIdCounter;
}
