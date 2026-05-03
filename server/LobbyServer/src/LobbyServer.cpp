#include "LobbyServer.h"
#include "LobbyImp.h"

using namespace std;
using namespace GameDemo;

LobbyServerApp g_app;

///////////////////////////////////////////////////////////////
void LobbyServerApp::initialize()
{
    TLOG_DEBUG("LobbyServerApp::initialize" << endl);
    
    // 注册 LobbyServant 接口 (客户端调用)
    addServant<LobbyImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".LobbyObj");
    
    // 注册 Scene2LobbyPush 接口 (SceneServer 调用)
    addServant<Scene2LobbyPushImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".Scene2LobbyPushObj");
}

void LobbyServerApp::destroyApp()
{
    TLOG_DEBUG("LobbyServerApp::destroyApp" << endl);
    // 清理推送相关数据
    _connToPlayer.clear();
    _playerToConn.clear();
    _playerCurrents.clear();
    _scenePlayers.clear();
}

// ========== 推送管理实现 ==========

void LobbyServerApp::registerClientPush(tars::Int64 playerId, tars::TarsCurrentPtr current)
{
    lock_guard<mutex> lock(_pushMutex);
    _playerCurrents[playerId] = current;
    TLOG_INFO("registerClientPush playerId=" << playerId << ", total=" << _playerCurrents.size() << endl);
}

void LobbyServerApp::addScenePlayer(tars::Int32 sceneId, tars::Int64 playerId)
{
    lock_guard<mutex> lock(_pushMutex);
    _scenePlayers[sceneId].insert(playerId);
    TLOG_INFO("addScenePlayer sceneId=" << sceneId << ", playerId=" << playerId << endl);
}

void LobbyServerApp::removeScenePlayer(tars::Int32 sceneId, tars::Int64 playerId)
{
    lock_guard<mutex> lock(_pushMutex);
    auto it = _scenePlayers.find(sceneId);
    if (it != _scenePlayers.end())
    {
        it->second.erase(playerId);
        if (it->second.empty())
        {
            _scenePlayers.erase(it);
        }
    }
    TLOG_INFO("removeScenePlayer sceneId=" << sceneId << ", playerId=" << playerId << endl);
}

void LobbyServerApp::broadcastToScene(tars::Int32 sceneId, tars::Int64 excludePlayerId, const PushCallback& callback)
{
    lock_guard<mutex> lock(_pushMutex);
    auto it = _scenePlayers.find(sceneId);
    if (it == _scenePlayers.end())
    {
        TLOG_WARN("broadcastToScene: no players in scene " << sceneId << endl);
        return;
    }

    int pushed = 0;
    for (tars::Int64 pid : it->second)
    {
        if (pid == excludePlayerId) continue;  // 排除发送者自己

        auto cit = _playerCurrents.find(pid);
        if (cit != _playerCurrents.end() && cit->second)
        {
            try
            {
                callback(cit->second);
                ++pushed;
            }
            catch (const std::exception& e)
            {
                TLOG_ERROR("broadcastToScene failed for playerId=" << pid << ": " << e.what() << endl);
            }
        }
    }
    TLOG_INFO("broadcastToScene sceneId=" << sceneId << ", exclude=" << excludePlayerId << ", pushed=" << pushed << endl);
}

void LobbyServerApp::bindConnId(tars::Int64 connId, tars::Int64 playerId)
{
    lock_guard<mutex> lock(_connMutex);
    _connToPlayer[connId] = playerId;
    _playerToConn[playerId] = connId;
}

void LobbyServerApp::unbindConnId(tars::Int64 connId)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _connToPlayer.find(connId);
    if (it != _connToPlayer.end())
    {
        _playerToConn.erase(it->second);
        _connToPlayer.erase(it);
    }
}

tars::Int64 LobbyServerApp::getPlayerIdByConnId(tars::Int64 connId)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _connToPlayer.find(connId);
    return (it != _connToPlayer.end()) ? it->second : 0;
}

tars::Int64 LobbyServerApp::getConnIdByPlayerId(tars::Int64 playerId)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _playerToConn.find(playerId);
    return (it != _playerToConn.end()) ? it->second : 0;
}

///////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    try
    {
        g_app.main(argc, argv);
        g_app.waitForShutdown();
    }
    catch (std::exception& e)
    {
        cerr << "std::exception:" << e.what() << std::endl;
    }
    catch (...)
    {
        cerr << "unknown exception." << std::endl;
    }
    return -1;
}
///////////////////////////////////////////////////////////////
