#ifndef _SceneServer_H_
#define _SceneServer_H_

#include <iostream>
#include <map>
#include <mutex>
#include "servant/Application.h"

using namespace tars;

/**
 * 全局共享的玩家数据管理器
 */
struct GlobalPlayerData
{
    tars::Int64 playerId;
    tars::Int32 sceneId;
    tars::Int32 level;
    float x, y, z;
    tars::Int64 lastHeartbeat;
};

class SceneServer : public Application
{
public:
    ~SceneServer() {};

    // 全局玩家数据访问接口
    std::map<tars::Int64, GlobalPlayerData>& getGlobalPlayers();
    
    void addPlayer(tars::Int64 playerId, const GlobalPlayerData& data);
    void removePlayer(tars::Int64 playerId);
    GlobalPlayerData* getPlayer(tars::Int64 playerId);
    void updatePlayerPosition(tars::Int64 playerId, float x, float y, float z);
    void updateHeartbeat(tars::Int64 playerId);

private:
    std::map<tars::Int64, GlobalPlayerData> _globalPlayers;
    std::mutex _globalMutex;
    
public:
    virtual void initialize();
    virtual void destroyApp();
};

extern SceneServer g_app;

///////////////////////////////////////////
#endif
