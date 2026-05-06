#ifndef _PlayerManager_H_
#define _PlayerManager_H_

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <pthread.h>
#include <cstdint>
#include <time.h>
#include "GameDemoBase.h"
#include "DB.h"

using namespace GameDemo;

/**
 * 玩家数据
 */
struct GlobalPlayerData
{
    int64_t playerId;
    int32_t sceneId;
    int32_t level;
    float x, y, z;
    bool isOnline;  // 是否在线（用于断线重连）
};

/**
 * 玩家管理器 - 统一管理玩家数据和 AOI 九宫格索引
 * 
 * 功能：
 * - 维护玩家完整数据 (playerId -> GlobalPlayerData)
 * - 维护格子 -> 玩家ID 的 AOI 索引
 * - 提供九宫格邻居查询
 * - 单一读写锁保护所有数据
 */
class PlayerManager
{
public:
    PlayerManager();
    ~PlayerManager();

    // ========== 对外接口 ==========

    /**
     * 玩家进入场景（一次性完成数据和 AOI 格子添加）
     */
    void playerEnter(int64_t playerId, float x, float y, float z, 
                     int32_t sceneId, int32_t level);

    /**
     * 玩家移动（一次性完成数据和 AOI 格子更新）
     * 同时异步保存位置到 Redis
     */
    void playerMove(int64_t playerId, float x, float y, float z);

    /**
     * 玩家离开场景（一次性完成 AOI 格子移除和数据移除）
     * 离开前同步保存数据到 Redis
     */
    void playerLeave(int64_t playerId);

    /**
     * 获取视野内玩家列表（不含自己）
     */
    std::vector<int64_t> getViewPlayers(int64_t playerId);

    /**
     * 获取玩家数据
     */
    GlobalPlayerData* getPlayer(int64_t playerId);

    /**
     * 设置玩家在线状态（用于断线重连）
     * 注意：断线和重连不改变 AOI 结构，只改变此标志
     */
    void setOnline(int64_t playerId, bool online);

    /**
     * 将玩家数据转换为 PlayerBaseInfo（用于通知）
     */
    PlayerBaseInfo toPlayerBaseInfo(const GlobalPlayerData& data);

    // ========== V0.5 Redis 持久化集成 ==========

    /**
     * 初始化 KV 代理（由 SceneServer 调用）
     */
    void setKVPrx(const KVServantPrx& kvPrx);

    /**
     * 进入场景恢复数据
     * - 内存无：从 Redis 恢复/初始化
     * - 已在内存且同场景：只设置在线状态
     * - 已在内存但跨场景：切换场景
     * 返回：是否需要通知旧场景玩家离开
     */
    bool restoreOnEnter(int64_t playerId, int32_t targetSceneId);

    /**
     * 玩家断线处理（必须同步保存）
     */
    void onPlayerOffline(int64_t playerId);

    /**
     * 定时全量快照（后台线程，定期保存所有在线玩家数据）
     */
    void startPeriodicSnapshot(int intervalSeconds = 30);

    // ========== 初始化/清理 ==========

    void init(int sceneWidth, int sceneHeight, float gridSize);
    void clear();

private:
    // ========== Redis 持久化辅助 ==========
    void savePlayerDataSync(int64_t playerId);
    static void* snapshotThreadFunc(void* arg);

    // ========== 内部辅助 ==========
    // 从 AOI 格子中移除玩家（不保存数据，不删除玩家数据，用于跨场景切换）
    void removeFromAOI(int64_t playerId);

private:
    // ========== 辅助函数 ==========

    std::pair<int, int> posToGrid(float x, float y) const;
    int gridToId(int gx, int gy) const;
    std::vector<int> getNeighborGrids(int gx, int gy) const;
    int calcGridId(float x, float y) const;

private:
    float _gridSize;           // 格子大小
    int _sceneWidth;           // 场景宽度
    int _sceneHeight;          // 场景高度
    int _gridXCount;           // X方向格子数
    int _gridYCount;           // Y方向格子数

    std::unordered_map<int64_t, GlobalPlayerData> _players;      // 玩家数据
    std::unordered_map<int, std::unordered_set<int64_t>> _grids; // 格子 -> 玩家ID列表
    mutable pthread_rwlock_t _rwlock;                                // 统一读写锁

    // ========== V0.5 Redis 持久化 ==========
    KVServantPrx _kvPrx;           // DBServer KVServant 代理
    pthread_t _snapshotThread;     // 定时快照线程
    bool _snapshotRunning;         // 快照线程运行标志
    int _snapshotInterval;         // 快照间隔（秒）
    pthread_rwlock_t _snapshotLock; // 快照线程锁
};

#endif
