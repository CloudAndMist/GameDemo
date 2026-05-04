#ifndef _SceneServer_H_
#define _SceneServer_H_

#include "servant/Application.h"
#include "PlayerManager.h"

class SceneServer : public Application
{
public:
    ~SceneServer() {};

    // 获取玩家管理器（全局单例，供所有 SceneImp 共享）
    PlayerManager& getPlayerManager() { return _playerManager; }

public:
    virtual void initialize();
    virtual void destroyApp();

private:
    PlayerManager _playerManager;  // 全局唯一的玩家管理器
};

extern SceneServer g_app;

#endif
