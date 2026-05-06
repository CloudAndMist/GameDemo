#include "SceneServer.h"
#include "SceneImp.h"
#include "DB.h"

SceneServer g_app;

//////////////////////////////////////////////////////////////////////////
void SceneServer::initialize()
{
    TLOG_DEBUG("SceneServer::initialize" << endl);

    // 初始化玩家管理器（在 SceneServer 层初始化，确保全局唯一）
    // TODO: 配置化（从配置文件读取）
    _playerManager.init(1000, 1000, 10.0f);  // 场景 1000x1000，格子 10x10

    // V0.5: 初始化 KV 代理（通过服务发现连接 DBServer 的 KVServant）
    _playerManager.setKVPrx(Application::getCommunicator()->stringToProxy<GameDemo::KVServantPrx>(
        ServerConfig::Application + ".DBServer.KVObj"));
    
    // V0.5: 启动定时全量快照（每 30 秒）
    _playerManager.startPeriodicSnapshot(30);

    // 注册 SceneServant 接口
    addServant<SceneImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".SceneObj");
}

//////////////////////////////////////////////////////////////////////////
void SceneServer::destroyApp()
{
    TLOG_DEBUG("SceneServer::destroyApp" << endl);

    // 清空玩家管理器
    _playerManager.clear();
}

//////////////////////////////////////////////////////////////////////////
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
