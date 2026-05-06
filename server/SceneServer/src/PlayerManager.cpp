#include "PlayerManager.h"
#include "servant/Application.h"
#include <algorithm>
#include <cmath>

using namespace std;

PlayerManager::PlayerManager()
    : _gridSize(0.0f)
    , _sceneWidth(0)
    , _sceneHeight(0)
    , _gridXCount(0)
    , _gridYCount(0)
    , _snapshotRunning(false)
    , _snapshotInterval(30)
{
    pthread_rwlock_init(&_rwlock, NULL);
    pthread_rwlock_init(&_snapshotLock, NULL);
}

PlayerManager::~PlayerManager()
{
    // 停止快照线程
    if (_snapshotRunning) {
        _snapshotRunning = false;
        pthread_join(_snapshotThread, NULL);
    }
    pthread_rwlock_destroy(&_rwlock);
    pthread_rwlock_destroy(&_snapshotLock);
}

void PlayerManager::init(int sceneWidth, int sceneHeight, float gridSize)
{
    _sceneWidth = sceneWidth;
    _sceneHeight = sceneHeight;
    _gridSize = gridSize;

    _gridXCount = static_cast<int>(ceil(static_cast<float>(sceneWidth) / gridSize));
    _gridYCount = static_cast<int>(ceil(static_cast<float>(sceneHeight) / gridSize));

    TLOG_INFO("PlayerManager: initialized with sceneSize=" << sceneWidth << "x" << sceneHeight
              << ", gridSize=" << gridSize
              << ", gridCount=" << _gridXCount << "x" << _gridYCount << endl);
}

//===========================================================================
// 对外接口
//===========================================================================

void PlayerManager::playerEnter(int64_t playerId, float x, float y, float z,
                                 int32_t sceneId, int32_t level)
{
    pthread_rwlock_wrlock(&_rwlock);

    // 添加玩家数据
    GlobalPlayerData data;
    data.playerId = playerId;
    data.sceneId = sceneId;
    data.level = level;
    data.x = x;
    data.y = y;
    data.z = z;
    data.isOnline = true;  // 默认在线
    _players[playerId] = data;

    // 加入 AOI 格子
    int gridId = calcGridId(x, y);
    _grids[gridId].insert(playerId);

    pthread_rwlock_unlock(&_rwlock);

    TLOG_DEBUG("PlayerManager::playerEnter playerId=" << playerId 
              << " at (" << x << "," << y << "," << z << "), gridId=" << gridId << endl);
}

void PlayerManager::playerMove(int64_t playerId, float x, float y, float z)
{
    pthread_rwlock_wrlock(&_rwlock);

    // 获取玩家当前数据
    auto it = _players.find(playerId);
    if (it == _players.end())
    {
        pthread_rwlock_unlock(&_rwlock);
        TLOG_WARN("PlayerManager::playerMove playerId=" << playerId << " not found" << endl);
        return;
    }

    // 用旧位置计算旧格子（此时 x, y 还是旧值）
    int oldGridId = calcGridId(it->second.x, it->second.y);
    int newGridId = calcGridId(x, y);

    // 更新位置
    it->second.x = x;
    it->second.y = y;
    it->second.z = z;

    // 如果格子变了，更新 AOI 格子索引
    if (oldGridId != newGridId)
    {
        _grids[oldGridId].erase(playerId);
        _grids[newGridId].insert(playerId);
        TLOG_DEBUG("PlayerManager::playerMove playerId=" << playerId
                  << " from gridId=" << oldGridId << " to gridId=" << newGridId << endl);
    }

    pthread_rwlock_unlock(&_rwlock);

    // V0.5: 异步保存位置到 Redis（发完即忘，不阻塞）
    if (_kvPrx) {
        PlayerKVData data;
        data.playerId = playerId;
        data.x = x;
        data.y = y;
        data.z = z;
        data.sceneId = it->second.sceneId;
        data.level = it->second.level;
        data.updateTime = time(NULL);
        _kvPrx->async_savePlayerData(NULL, data);
    }
}

void PlayerManager::playerLeave(int64_t playerId)
{
    // V0.5: 离开前必须同步保存数据
    savePlayerDataSync(playerId);

    pthread_rwlock_wrlock(&_rwlock);

    // 获取玩家当前数据
    auto it = _players.find(playerId);
    if (it != _players.end())
    {
        // 用旧位置计算旧格子（此时 x, y 还是旧值）
        int gridId = calcGridId(it->second.x, it->second.y);
        _grids[gridId].erase(playerId);
        TLOG_DEBUG("PlayerManager::playerLeave playerId=" << playerId 
                  << " from gridId=" << gridId << endl);
    }

    // 移除玩家数据
    _players.erase(playerId);

    pthread_rwlock_unlock(&_rwlock);
}

void PlayerManager::setOnline(int64_t playerId, bool online)
{
    pthread_rwlock_wrlock(&_rwlock);

    auto it = _players.find(playerId);
    if (it != _players.end())
    {
        it->second.isOnline = online;
        TLOG_INFO("PlayerManager::setOnline playerId=" << playerId << ", isOnline=" << online << endl);
    }
    else
    {
        TLOG_WARN("PlayerManager::setOnline playerId=" << playerId << " not found" << endl);
    }

    pthread_rwlock_unlock(&_rwlock);
}

PlayerBaseInfo PlayerManager::toPlayerBaseInfo(const GlobalPlayerData& data)
{
    PlayerBaseInfo info;
    info.playerId = data.playerId;
    info.sceneId = data.sceneId;
    info.level = data.level;
    info.posX = data.x;
    info.posY = data.y;
    info.posZ = data.z;
    return info;
}

vector<int64_t> PlayerManager::getViewPlayers(int64_t playerId)
{
    vector<int64_t> viewPlayers;

    pthread_rwlock_rdlock(&_rwlock);

    // 获取玩家当前数据和场景
    auto it = _players.find(playerId);
    if (it == _players.end())
    {
        pthread_rwlock_unlock(&_rwlock);
        TLOG_WARN("PlayerManager::getViewPlayers playerId=" << playerId << " not found" << endl);
        return viewPlayers;
    }

    int32_t mySceneId = it->second.sceneId;

    // 计算当前格子
    auto pg = posToGrid(it->second.x, it->second.y);
    int gx = pg.first;
    int gy = pg.second;

    // 获取九宫格邻居格子
    vector<int> neighborGrids = getNeighborGrids(gx, gy);

    for (int gridId : neighborGrids)
    {
        auto git = _grids.find(gridId);
        if (git != _grids.end())
        {
            for (int64_t pid : git->second)
            {
                if (pid != playerId)  // 排除自己
                {
                    // 关键：只返回同一场景的玩家
                    auto pit = _players.find(pid);
                    if (pit != _players.end() && pit->second.sceneId == mySceneId)
                    {
                        viewPlayers.push_back(pid);
                    }
                }
            }
        }
    }

    pthread_rwlock_unlock(&_rwlock);

    return viewPlayers;
}

GlobalPlayerData* PlayerManager::getPlayer(int64_t playerId)
{
    pthread_rwlock_rdlock(&_rwlock);
    auto it = _players.find(playerId);
    GlobalPlayerData* result = (it != _players.end()) ? &it->second : nullptr;
    pthread_rwlock_unlock(&_rwlock);
    return result;
}

void PlayerManager::clear()
{
    pthread_rwlock_wrlock(&_rwlock);
    _players.clear();
    _grids.clear();
    pthread_rwlock_unlock(&_rwlock);

    TLOG_DEBUG("PlayerManager::clear" << endl);
}

//===========================================================================
// 辅助函数
//===========================================================================

pair<int, int> PlayerManager::posToGrid(float x, float y) const
{
    int gx = static_cast<int>(x / _gridSize);
    int gy = static_cast<int>(y / _gridSize);

    // 边界处理
    gx = max(0, min(gx, _gridXCount - 1));
    gy = max(0, min(gy, _gridYCount - 1));

    return {gx, gy};
}

int PlayerManager::gridToId(int gx, int gy) const
{
    return gy * _gridXCount + gx;
}

int PlayerManager::calcGridId(float x, float y) const
{
    auto pg = posToGrid(x, y);
    return gridToId(pg.first, pg.second);
}

vector<int> PlayerManager::getNeighborGrids(int gx, int gy) const
{
    vector<int> neighbors;
    neighbors.reserve(9);

    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            int nx = gx + dx;
            int ny = gy + dy;

            // 边界检查
            if (nx >= 0 && nx < _gridXCount && ny >= 0 && ny < _gridYCount)
            {
                neighbors.push_back(gridToId(nx, ny));
            }
        }
    }

    return neighbors;
}

//===========================================================================
// V0.5 Redis 持久化集成
//===========================================================================

void PlayerManager::setKVPrx(const KVServantPrx& kvPrx)
{
    _kvPrx = kvPrx;
    TLOG_INFO("PlayerManager::setKVPrx KV proxy set" << endl);
}

bool PlayerManager::restoreOnEnter(int64_t playerId, int32_t targetSceneId)
{
    // 从 Redis 恢复数据
    if (!_kvPrx) {
        TLOG_ERROR("PlayerManager::restoreOnEnter _kvPrx is null!" << endl);
        return false;
    }

    PlayerKVData dbData;
    int ret = _kvPrx->getPlayerData(playerId, dbData);
    
    if (ret != 0) {
        // 新玩家：在本 scene 初始化 + 同步保存
        TLOG_INFO("PlayerManager::restoreOnEnter new playerId=" << playerId << ", initializing" << endl);
        playerEnter(playerId, 0.0f, 0.0f, 0.0f, targetSceneId, 1);
        savePlayerDataSync(playerId);
        return false;
    }

    // 检查内存中是否有该玩家
    pthread_rwlock_rdlock(&_rwlock);
    bool inMemory = (_players.find(playerId) != _players.end());
    int32_t currentSceneId = inMemory ? _players.at(playerId).sceneId : dbData.sceneId;
    pthread_rwlock_unlock(&_rwlock);

    if (inMemory && currentSceneId == targetSceneId) {
        // 同场景已在内存中，只设置在线状态
        setOnline(playerId, true);
        TLOG_DEBUG("PlayerManager::restoreOnEnter playerId=" << playerId << " already in memory, set online" << endl);
        return false;
    }

    // 不在内存中，检查是否是同场景恢复
    if (currentSceneId == targetSceneId) {
        // 同场景恢复：从 Redis 恢复位置
        TLOG_INFO("PlayerManager::restoreOnEnter same-scene restore playerId=" << playerId 
                  << " at (" << dbData.x << ", " << dbData.y << ", " << dbData.z << ")" << endl);
        playerEnter(playerId, dbData.x, dbData.y, dbData.z, dbData.sceneId, dbData.level);
        return false;
    }

    // 跨场景（包括内存中不同场景 或 Redis 中不同场景）
    TLOG_INFO("PlayerManager::restoreOnEnter cross-scene playerId=" << playerId 
             << " from sceneId=" << currentSceneId << " to sceneId=" << targetSceneId << endl);
    
    // 如果在内存中，先从旧场景 AOI 移除（不保存数据，不删除玩家数据）
    if (inMemory) {
        removeFromAOI(playerId);
        TLOG_DEBUG("PlayerManager::restoreOnEnter removed from old scene AOI, sceneId=" << currentSceneId << endl);
    }
    
    // 进入新场景（跨场景不保留原位置，在入口点生成）
    playerEnter(playerId, 0.0f, 0.0f, 0.0f, targetSceneId, dbData.level);
    savePlayerDataSync(playerId);
    
    return true;  // 返回 true 表示发生了跨场景切换，需要通知
}

void PlayerManager::removeFromAOI(int64_t playerId)
{
    pthread_rwlock_wrlock(&_rwlock);
    
    auto it = _players.find(playerId);
    if (it != _players.end()) {
        int gridId = calcGridId(it->second.x, it->second.y);
        _grids[gridId].erase(playerId);
        TLOG_DEBUG("PlayerManager::removeFromAOI playerId=" << playerId << " from gridId=" << gridId << endl);
    }
    
    pthread_rwlock_unlock(&_rwlock);
}

void PlayerManager::onPlayerOffline(int64_t playerId)
{
    // 断线必须同步保存
    TLOG_INFO("PlayerManager::onPlayerOffline playerId=" << playerId << endl);
    savePlayerDataSync(playerId);
    setOnline(playerId, false);
}

void PlayerManager::savePlayerDataSync(int64_t playerId)
{
    if (!_kvPrx) return;

    pthread_rwlock_rdlock(&_rwlock);
    auto it = _players.find(playerId);
    if (it == _players.end()) {
        pthread_rwlock_unlock(&_rwlock);
        return;
    }

    PlayerKVData data;
    data.playerId = playerId;
    data.x = it->second.x;
    data.y = it->second.y;
    data.z = it->second.z;
    data.sceneId = it->second.sceneId;
    data.level = it->second.level;
    data.updateTime = time(NULL);
    pthread_rwlock_unlock(&_rwlock);

    // 同步调用，阻塞等待保存完成
    int ret = _kvPrx->savePlayerData(data);
    if (ret == 0) {
        TLOG_DEBUG("PlayerManager::savePlayerDataSync playerId=" << playerId << " saved" << endl);
    } else {
        TLOG_ERROR("PlayerManager::savePlayerDataSync playerId=" << playerId << " failed, ret=" << ret << endl);
    }
}

void* PlayerManager::snapshotThreadFunc(void* arg)
{
    PlayerManager* mgr = static_cast<PlayerManager*>(arg);
    TLOG_INFO("PlayerManager::snapshotThread started, interval=" << mgr->_snapshotInterval << "s" << endl);
    
    while (mgr->_snapshotRunning) {
        sleep(mgr->_snapshotInterval);
        if (!mgr->_snapshotRunning) break;

        pthread_rwlock_rdlock(&mgr->_rwlock);
        size_t playerCount = mgr->_players.size();
        if (playerCount == 0) {
            pthread_rwlock_unlock(&mgr->_rwlock);
            continue;
        }

        // 批量保存所有在线玩家数据
        vector<PlayerKVData> dataList;
        dataList.reserve(playerCount);
        for (const auto& kv : mgr->_players) {
            if (!kv.second.isOnline) continue;
            PlayerKVData data;
            data.playerId = kv.first;
            data.x = kv.second.x;
            data.y = kv.second.y;
            data.z = kv.second.z;
            data.sceneId = kv.second.sceneId;
            data.level = kv.second.level;
            data.updateTime = time(NULL);
            dataList.push_back(data);
        }
        pthread_rwlock_unlock(&mgr->_rwlock);

        if (!dataList.empty()) {
            int ret = mgr->_kvPrx->batchSavePlayerData(dataList);
            TLOG_INFO("PlayerManager::snapshot saved " << dataList.size() << " players, ret=" << ret << endl);
        }
    }
    
    TLOG_INFO("PlayerManager::snapshotThread exited" << endl);
    return NULL;
}

void PlayerManager::startPeriodicSnapshot(int intervalSeconds)
{
    if (_snapshotRunning) {
        TLOG_WARN("PlayerManager::startPeriodicSnapshot already running" << endl);
        return;
    }

    if (!_kvPrx) {
        TLOG_ERROR("PlayerManager::startPeriodicSnapshot _kvPrx is null, cannot start" << endl);
        return;
    }

    _snapshotInterval = intervalSeconds;
    _snapshotRunning = true;
    
    if (pthread_create(&_snapshotThread, NULL, snapshotThreadFunc, this) != 0) {
        TLOG_ERROR("PlayerManager::startPeriodicSnapshot pthread_create failed" << endl);
        _snapshotRunning = false;
        return;
    }
    
    TLOG_INFO("PlayerManager::startPeriodicSnapshot started, interval=" << intervalSeconds << "s" << endl);
}
