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
    
    // V0.4: 初始化 Session 读写锁
    pthread_rwlock_init(&_sessionsRwlock, NULL);
    
    // 注册 LobbyServant 接口 (客户端调用)
    addServant<LobbyImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".LobbyObj");
    
    // 注册 Scene2LobbyPush 接口 (SceneServer 调用)
    addServant<Scene2LobbyPushImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".Scene2LobbyPushObj");
    
    // V0.4: 初始化 SceneServer 代理（用于通知玩家掉线/重连）
    try
    {
        _scenePrx = Application::getCommunicator()->stringToProxy<GameDemo::SceneServantPrx>(
            "GameDemo.SceneServer.SceneObj"
        );
        TLOG_INFO("LobbyServerApp: SceneServer proxy initialized" << endl);
    }
    catch (exception& e)
    {
        TLOG_ERROR("LobbyServerApp: Failed to init SceneServer proxy: " << e.what() << endl);
    }
    
    // V0.4: 启动心跳检测线程 (使用 Tars TC_Thread)
    _heartbeatThread.setApp(this);
    _heartbeatThread.start();
}

void LobbyServerApp::destroyApp()
{
    TLOG_DEBUG("LobbyServerApp::destroyApp" << endl);
    
    // V0.4: 停止心跳检测线程
    _heartbeatThread.terminate();
    _heartbeatThread.join();
    
    // 销毁读写锁 (V0.2.5)
    pthread_rwlock_destroy(&_playerCurrentsRwlock);
    
    // V0.4: 销毁 Session 读写锁
    pthread_rwlock_destroy(&_sessionsRwlock);
    
    // 清理推送相关数据
    _playerCurrents.clear();
    
    // V0.4: 清理 Session 数据
    _sessions.clear();
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

// ========== Session 管理实现 (V0.4) ==========

void LobbyServerApp::createSession(tars::Int64 playerId, tars::Int64 sessionKey)
{
    pthread_rwlock_wrlock(&_sessionsRwlock);
    
    PlayerSession session;
    session.playerId = playerId;
    session.sessionKey = sessionKey;
    session.lastHeartbeat = TNOW;
    session.isOnline = true;
    session.sceneId = -1;           // V0.4: 初始无场景（-1 表示不在任何场景）
    session.offlineTime = 0;
    
    _sessions[playerId] = session;
    
    TLOG_INFO("createSession playerId=" << playerId << ", sessionKey=" << sessionKey 
              << ", total=" << _sessions.size() << endl);
    
    pthread_rwlock_unlock(&_sessionsRwlock);
}

void LobbyServerApp::updateHeartbeat(tars::Int64 playerId)
{
    pthread_rwlock_wrlock(&_sessionsRwlock);
    
    auto it = _sessions.find(playerId);
    if (it != _sessions.end())
    {
        it->second.lastHeartbeat = TNOW;
        
        // 如果之前是离线状态，重连时恢复在线
        if (!it->second.isOnline)
        {
            it->second.isOnline = true;
            it->second.offlineTime = 0;
            
            tars::Int32 sceneId = it->second.sceneId;
            TLOG_INFO("updateHeartbeat: playerId=" << playerId << " reconnected, isOnline=true, sceneId=" << sceneId << endl);
            
            pthread_rwlock_unlock(&_sessionsRwlock);
            
            // V0.4: 通知 SceneServer 玩家重连
            if (sceneId > 0 && _scenePrx)
            {
                try
                {
                    _scenePrx->async_playerOnline(NULL, playerId, sceneId);
                    TLOG_INFO("updateHeartbeat: notified SceneServer playerOnline playerId=" << playerId << ", sceneId=" << sceneId << endl);
                }
                catch (exception& e)
                {
                    TLOG_ERROR("updateHeartbeat: failed to notify SceneServer playerOnline playerId=" << playerId << ": " << e.what() << endl);
                }
            }
            return;
        }
    }
    
    pthread_rwlock_unlock(&_sessionsRwlock);
}

bool LobbyServerApp::validateSession(tars::Int64 playerId, tars::Int64 sessionKey)
{
    pthread_rwlock_rdlock(&_sessionsRwlock);
    
    auto it = _sessions.find(playerId);
    bool valid = (it != _sessions.end() && it->second.sessionKey == sessionKey);
    
    pthread_rwlock_unlock(&_sessionsRwlock);
    
    return valid;
}

PlayerSession* LobbyServerApp::getSession(tars::Int64 playerId)
{
    pthread_rwlock_rdlock(&_sessionsRwlock);
    
    auto it = _sessions.find(playerId);
    PlayerSession* result = (it != _sessions.end()) ? &it->second : nullptr;
    
    pthread_rwlock_unlock(&_sessionsRwlock);
    
    return result;
}

void LobbyServerApp::updateSceneId(tars::Int64 playerId, tars::Int32 sceneId)
{
    pthread_rwlock_wrlock(&_sessionsRwlock);
    
    auto it = _sessions.find(playerId);
    if (it != _sessions.end())
    {
        it->second.sceneId = sceneId;
        TLOG_INFO("updateSceneId playerId=" << playerId << ", sceneId=" << sceneId << endl);
    }
    
    pthread_rwlock_unlock(&_sessionsRwlock);
}

tars::Int32 LobbyServerApp::getSessionSceneId(tars::Int64 playerId)
{
    pthread_rwlock_rdlock(&_sessionsRwlock);
    
    auto it = _sessions.find(playerId);
    tars::Int32 sceneId = -1;  // 默认不在任何场景
    
    if (it != _sessions.end())
    {
        sceneId = it->second.sceneId;
    }
    
    pthread_rwlock_unlock(&_sessionsRwlock);
    
    return sceneId;
}

void LobbyServerApp::setPlayerOffline(tars::Int64 playerId)
{
    pthread_rwlock_wrlock(&_sessionsRwlock);
    
    auto it = _sessions.find(playerId);
    if (it != _sessions.end())
    {
        it->second.isOnline = false;
        it->second.offlineTime = TNOW;
        TLOG_INFO("setPlayerOffline playerId=" << playerId << ", isOnline=false, offlineTime=" << it->second.offlineTime << endl);
    }
    
    pthread_rwlock_unlock(&_sessionsRwlock);
}

void LobbyServerApp::setPlayerOnline(tars::Int64 playerId)
{
    pthread_rwlock_wrlock(&_sessionsRwlock);
    
    auto it = _sessions.find(playerId);
    if (it != _sessions.end())
    {
        it->second.isOnline = true;
        it->second.offlineTime = 0;
        TLOG_INFO("setPlayerOnline playerId=" << playerId << ", isOnline=true" << endl);
    }
    
    pthread_rwlock_unlock(&_sessionsRwlock);
}

void LobbyServerApp::removePlayer(tars::Int64 playerId)
{
    // 清理 Session
    pthread_rwlock_wrlock(&_sessionsRwlock);
    _sessions.erase(playerId);
    TLOG_INFO("removePlayer playerId=" << playerId << ", remaining sessions=" << _sessions.size() << endl);
    pthread_rwlock_unlock(&_sessionsRwlock);

    // 清理推送连接
    pthread_rwlock_wrlock(&_playerCurrentsRwlock);
    _playerCurrents.erase(playerId);
    TLOG_INFO("removePlayer playerId=" << playerId << ", remaining currents=" << _playerCurrents.size() << endl);
    pthread_rwlock_unlock(&_playerCurrentsRwlock);
}

vector<tars::Int64> LobbyServerApp::getOnlinePlayers()
{
    vector<tars::Int64> onlinePlayers;
    
    pthread_rwlock_rdlock(&_sessionsRwlock);
    
    for (const auto& pair : _sessions)
    {
        if (pair.second.isOnline)
        {
            onlinePlayers.push_back(pair.first);
        }
    }
    
    pthread_rwlock_unlock(&_sessionsRwlock);
    
    return onlinePlayers;
}

// ========== 心跳超时检测 (V0.4) - HeartbeatThread 已独立到 HeartbeatThread.cpp ==========

void LobbyServerApp::checkHeartbeatTimeout()
{
    const tars::Int64 TIMEOUT_THRESHOLD = 15;  // 15 秒超时判定离线
    const tars::Int64 CLEANUP_THRESHOLD = 30;   // 30 秒后彻底清除 Session
    
    tars::Int64 now = TNOW;
    
    pthread_rwlock_wrlock(&_sessionsRwlock);
    
    vector<tars::Int64> toOffline;      // 需要标记离线的玩家
    vector<tars::Int64> toCleanup;      // 需要彻底清除的玩家
    
    for (auto& pair : _sessions)
    {
        PlayerSession& session = pair.second;
        
        if (session.isOnline)
        {
            // 在线玩家检测心跳超时
            if (session.lastHeartbeat < now - TIMEOUT_THRESHOLD)
            {
                toOffline.push_back(pair.first);
            }
        }
        else
        {
            // 离线玩家检测是否需要清除
            if (session.offlineTime > 0 && session.offlineTime < now - CLEANUP_THRESHOLD)
            {
                toCleanup.push_back(pair.first);
            }
        }
    }
    
    pthread_rwlock_unlock(&_sessionsRwlock);
    
    // 处理需要标记离线的玩家
    for (tars::Int64 playerId : toOffline)
    {
        TLOG_INFO("checkHeartbeatTimeout: playerId=" << playerId << " timeout" << endl);
        
        // V0.4: 获取该玩家所在的场景ID，并标记为离线
        tars::Int32 sceneId = getSessionSceneId(playerId);
        
        // 标记为离线状态
        pthread_rwlock_wrlock(&_sessionsRwlock);
        auto it = _sessions.find(playerId);
        if (it != _sessions.end())
        {
            it->second.isOnline = false;
            it->second.offlineTime = now;
        }
        pthread_rwlock_unlock(&_sessionsRwlock);
        
        // V0.4: 通知 SceneServer 玩家掉线（仅当玩家在场景中时）
        if (sceneId > 0 && _scenePrx)
        {
            try
            {
                _scenePrx->async_playerOffline(NULL, playerId, sceneId);
                TLOG_INFO("checkHeartbeatTimeout: notified SceneServer playerOffline playerId=" << playerId << ", sceneId=" << sceneId << endl);
            }
            catch (exception& e)
            {
                TLOG_ERROR("checkHeartbeatTimeout: failed to notify SceneServer playerOffline playerId=" << playerId << ": " << e.what() << endl);
            }
        }
        
        TLOG_INFO("checkHeartbeatTimeout: playerId=" << playerId << " marked offline, sceneId=" << sceneId);
    }
    
    // 处理需要清除 Session 的玩家（30秒后彻底清理）
    for (tars::Int64 playerId : toCleanup)
    {
        // V0.4: 获取场景ID，通知 SceneServer 玩家离开场景
        tars::Int32 sceneId = getSessionSceneId(playerId);
        
        if (sceneId > 0 && _scenePrx)
        {
            try
            {
                LeaveSceneReq req;
                req.playerId = playerId;
                req.sceneId = sceneId;
                _scenePrx->async_leaveScene(NULL, req);
                TLOG_INFO("checkHeartbeatTimeout: notified SceneServer leaveScene playerId=" << playerId << ", sceneId=" << sceneId << endl);
            }
            catch (exception& e)
            {
                TLOG_ERROR("checkHeartbeatTimeout: failed to notify SceneServer leaveScene playerId=" << playerId << ": " << e.what() << endl);
            }
        }
        
        // 复用 removePlayer 清理 Session + 推送连接
        removePlayer(playerId);
    }
    
    if (!toOffline.empty() || !toCleanup.empty())
    {
        TLOG_INFO("checkHeartbeatTimeout: toOffline=" << toOffline.size() << ", toCleanup=" << toCleanup.size() << endl);
    }
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
