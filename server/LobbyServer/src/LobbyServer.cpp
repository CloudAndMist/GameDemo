#include "LobbyServer.h"
#include "LobbyImp.h"

using namespace std;
using namespace GameDemo;

LobbyServerApp g_app;

/////////////////////////////////////////////////////////////////
void LobbyServerApp::initialize()
{
    TLOG_DEBUG("LobbyServerApp::initialize" << endl);
    addServant<LobbyImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".LobbyObj");
}

void LobbyServerApp::destroyApp()
{
    TLOG_DEBUG("LobbyServerApp::destroyApp" << endl);
    lock_guard<mutex> lock(_mutex);
    _connToPlayer.clear();
    _playerToConn.clear();
}

void LobbyServerApp::bindConnId(tars::Int64 connId, tars::Int64 playerId)
{
    lock_guard<mutex> lock(_mutex);
    _connToPlayer[connId] = playerId;
    _playerToConn[playerId] = connId;
}

void LobbyServerApp::unbindConnId(tars::Int64 connId)
{
    lock_guard<mutex> lock(_mutex);
    auto it = _connToPlayer.find(connId);
    if (it != _connToPlayer.end())
    {
        _playerToConn.erase(it->second);
        _connToPlayer.erase(it);
    }
}

tars::Int64 LobbyServerApp::getPlayerIdByConnId(tars::Int64 connId)
{
    lock_guard<mutex> lock(_mutex);
    auto it = _connToPlayer.find(connId);
    return (it != _connToPlayer.end()) ? it->second : 0;
}

tars::Int64 LobbyServerApp::getConnIdByPlayerId(tars::Int64 playerId)
{
    lock_guard<mutex> lock(_mutex);
    auto it = _playerToConn.find(playerId);
    return (it != _playerToConn.end()) ? it->second : 0;
}

/////////////////////////////////////////////////////////////////
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
/////////////////////////////////////////////////////////////////
