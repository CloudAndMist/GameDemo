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
{
    pthread_rwlock_init(&_rwlock, NULL);
}

PlayerManager::~PlayerManager()
{
    pthread_rwlock_destroy(&_rwlock);
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
}

void PlayerManager::playerLeave(int64_t playerId)
{
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

vector<int64_t> PlayerManager::getViewPlayers(int64_t playerId)
{
    vector<int64_t> viewPlayers;

    pthread_rwlock_rdlock(&_rwlock);

    // 获取玩家当前格子
    auto it = _players.find(playerId);
    if (it == _players.end())
    {
        pthread_rwlock_unlock(&_rwlock);
        TLOG_WARN("PlayerManager::getViewPlayers playerId=" << playerId << " not found" << endl);
        return viewPlayers;
    }

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
                    viewPlayers.push_back(pid);
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
