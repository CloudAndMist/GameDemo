#include "LobbyServer.h"
#include "LobbyImp.h"

using namespace std;
using namespace GameDemo;

LobbyServerApp g_app;

///////////////////////////////////////////////////////////////
void LobbyServerApp::initialize()
{
    TLOG_DEBUG("LobbyServerApp::initialize" << endl);
    
    // 初始化读写锁 (V0.2.5)
    pthread_rwlock_init(&_playerCurrentsRwlock, NULL);
    
    // 注册 LobbyServant 接口 (客户端调用)
    addServant<LobbyImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".LobbyObj");
    
    // 注册 Scene2LobbyPush 接口 (SceneServer 调用)
    addServant<Scene2LobbyPushImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".Scene2LobbyPushObj");
}

void LobbyServerApp::destroyApp()
{
    TLOG_DEBUG("LobbyServerApp::destroyApp" << endl);
    // 销毁读写锁 (V0.2.5)
    pthread_rwlock_destroy(&_playerCurrentsRwlock);
    // 清理推送相关数据
    _connToPlayer.clear();
    _playerToConn.clear();
    _playerCurrents.clear();
}

// ========== 推送管理实现 (V0.2.5 pthread 读写锁优化) ==========

void LobbyServerApp::registerClientPush(tars::Int64 playerId, tars::TarsCurrentPtr current)
{
    // 写操作：独占锁
    pthread_rwlock_wrlock(&_playerCurrentsRwlock);
    _playerCurrents[playerId] = current;
    TLOG_INFO("registerClientPush playerId=" << playerId << ", total=" << _playerCurrents.size() << endl);
    pthread_rwlock_unlock(&_playerCurrentsRwlock);
}

tars::TarsCurrentPtr LobbyServerApp::getPlayerCurrent(tars::Int64 playerId)
{
    // 读操作：共享锁
    pthread_rwlock_rdlock(&_playerCurrentsRwlock);
    auto it = _playerCurrents.find(playerId);
    tars::TarsCurrentPtr result = (it != _playerCurrents.end()) ? it->second : nullptr;
    pthread_rwlock_unlock(&_playerCurrentsRwlock);
    return result;
}

// V0.2.5: 根据 notifyList 推送 - pthread 读写锁优化
// SceneServer 传入需要通知的玩家列表，LobbyServer 只负责根据列表推送
// 读操作使用共享锁，多个推送可并行执行
void LobbyServerApp::pushToNotifyList(const vector<tars::Int64>& notifyList, const PushCallback& callback)
{
    // 读操作：共享锁 - 多个 pushToNotifyList 可同时执行
    pthread_rwlock_rdlock(&_playerCurrentsRwlock);

    int pushed = 0;
    int total = notifyList.size();
    
    for (tars::Int64 targetPlayerId : notifyList)
    {
        auto it = _playerCurrents.find(targetPlayerId);
        if (it == _playerCurrents.end() || !it->second)
        {
            continue;
        }

        try
        {
            // 回调执行在锁内，但回调本身是异步的（async_response_*）
            // 这里只是调用发起，发送完成后立即返回
            callback(it->second);
            ++pushed;
        }
        catch (const std::exception& e)
        {
            TLOG_ERROR("pushToNotifyList failed for playerId=" << targetPlayerId << ": " << e.what() << endl);
        }
    }
    pthread_rwlock_unlock(&_playerCurrentsRwlock);
    
    TLOG_DEBUG("pushToNotifyList pushed=" << pushed << "/" << total << endl);
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
