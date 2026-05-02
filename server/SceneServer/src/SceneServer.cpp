#include "SceneServer.h"
#include "SceneImp.h"
#include <chrono>

using namespace std;

SceneServer g_app;

/////////////////////////////////////////////////////////////
std::map<tars::Int64, GlobalPlayerData>& SceneServer::getGlobalPlayers()
{
    return _globalPlayers;
}

void SceneServer::addPlayer(tars::Int64 playerId, const GlobalPlayerData& data)
{
    lock_guard<mutex> lock(_globalMutex);
    _globalPlayers[playerId] = data;
}

void SceneServer::removePlayer(tars::Int64 playerId)
{
    lock_guard<mutex> lock(_globalMutex);
    _globalPlayers.erase(playerId);
}

GlobalPlayerData* SceneServer::getPlayer(tars::Int64 playerId)
{
    lock_guard<mutex> lock(_globalMutex);
    auto it = _globalPlayers.find(playerId);
    if (it != _globalPlayers.end())
    {
        return &it->second;
    }
    return nullptr;
}

void SceneServer::updatePlayerPosition(tars::Int64 playerId, float x, float y, float z)
{
    lock_guard<mutex> lock(_globalMutex);
    auto it = _globalPlayers.find(playerId);
    if (it != _globalPlayers.end())
    {
        it->second.x = x;
        it->second.y = y;
        it->second.z = z;
    }
}

void SceneServer::updateHeartbeat(tars::Int64 playerId)
{
    lock_guard<mutex> lock(_globalMutex);
    auto it = _globalPlayers.find(playerId);
    if (it != _globalPlayers.end())
    {
        it->second.lastHeartbeat = chrono::duration_cast<chrono::milliseconds>(
            chrono::system_clock::now().time_since_epoch()).count();
    }
}

/////////////////////////////////////////////////////////////
void SceneServer::initialize()
{
    TLOG_DEBUG("SceneServer::initialize" << endl);
    addServant<SceneImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".SceneObj");
}

/////////////////////////////////////////////////////////////
void SceneServer::destroyApp()
{
    TLOG_DEBUG("SceneServer::destroyApp" << endl);
    lock_guard<mutex> lock(_globalMutex);
    _globalPlayers.clear();
}

/////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
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
