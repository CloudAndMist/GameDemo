#include "KVImp.h"
#include "util/tc_common.h"

using namespace std;

const char* KV_KEY_PREFIX = "player:kv:";
const int DEFAULT_TTL = 86400;  // 24小时

void KVImp::initialize()
{
    TLOG_DEBUG("[KVImp::initialize]" << endl);
    
    TC_Config &conf = getApplication()->getConfig();
    _redisHost = conf.get("/kv<host>", "tars-redis");
    _redisPort = TC_Common::strto<int>(conf.get("/kv<port>", "6379"));
    _dataTTL = TC_Common::strto<int>(conf.get("/kv<ttl>", "86400"));
    
    TLOG_DEBUG("[KVImp] Redis config: host=" << _redisHost << ", port=" << _redisPort << endl);

    if (!connectRedis()) {
        TLOG_ERROR("KVImp: Failed to connect to Redis!" << endl);
    } else {
        TLOG_DEBUG("[KVImp] Redis connected successfully!" << endl);
    }
}

void KVImp::destroy()
{
    if (_redis) {
        redisFree(_redis);
        _redis = nullptr;
    }
}

bool KVImp::connectRedis()
{
    _redis = redisConnect(_redisHost.c_str(), _redisPort);
    if (_redis == nullptr || _redis->err) {
        TLOG_ERROR("KVImp: Redis error: " << (_redis ? _redis->errstr : "malloc failed") << endl);
        if (_redis) { redisFree(_redis); _redis = nullptr; }
        return false;
    }
    return true;
}

string KVImp::playerKey(tars::Int64 playerId)
{
    return string(KV_KEY_PREFIX) + TC_Common::tostr(playerId) + ":";
}

tars::Int32 KVImp::savePlayerData(const GameDemo::PlayerKVData& data, tars::TarsCurrentPtr current)
{
    TLOG_DEBUG("[KVImp::savePlayerData] playerId=" << data.playerId << endl);
    if (!_redis) {
        TLOG_ERROR("[KVImp::savePlayerData] Redis not connected!" << endl);
        return -1;
    }

    string key = playerKey(data.playerId);

    redisReply *reply = (redisReply*)redisCommand(_redis,
        "HSET %s level %d x %f y %f z %f sceneId %d updateTime %ld",
        key.c_str(), data.level, data.x, data.y, data.z, data.sceneId, data.updateTime);

    if (reply) freeReplyObject(reply);
    redisCommand(_redis, "EXPIRE %s %d", key.c_str(), _dataTTL);

    TLOG_DEBUG("[KVImp::savePlayerData] done" << endl);
    return 0;
}

tars::Int32 KVImp::batchSavePlayerData(const vector<GameDemo::PlayerKVData>& dataList, tars::TarsCurrentPtr current)
{
    if (!_redis) return -1;

    for (const auto& data : dataList) {
        string key = playerKey(data.playerId);
        redisReply *reply = (redisReply*)redisCommand(_redis,
            "HSET %s level %d x %f y %f z %f sceneId %d updateTime %ld",
            key.c_str(), data.level, data.x, data.y, data.z, data.sceneId, data.updateTime);
        if (reply) freeReplyObject(reply);

        redisCommand(_redis, "EXPIRE %s %d", key.c_str(), _dataTTL);
    }
    return 0;
}

tars::Int32 KVImp::getPlayerData(tars::Int64 playerId, GameDemo::PlayerKVData& data, tars::TarsCurrentPtr current)
{
    if (!_redis) return -1;

    redisReply *reply = (redisReply*)redisCommand(_redis, "HGETALL %s", playerKey(playerId).c_str());
    if (!reply || reply->type != REDIS_REPLY_ARRAY || reply->elements == 0) {
        if (reply) freeReplyObject(reply);
        return GameDemo::ERR_KV_NOT_FOUND;
    }

    data.playerId = playerId;
    for (size_t i = 0; i < reply->elements; i += 2) {
        string field = reply->element[i]->str;
        string value = reply->element[i + 1]->str;

        if (field == "level") data.level = TC_Common::strto<int>(value);
        else if (field == "x") data.x = TC_Common::strto<float>(value);
        else if (field == "y") data.y = TC_Common::strto<float>(value);
        else if (field == "z") data.z = TC_Common::strto<float>(value);
        else if (field == "sceneId") data.sceneId = TC_Common::strto<int>(value);
        else if (field == "updateTime") data.updateTime = TC_Common::strto<long>(value);
    }

    freeReplyObject(reply);
    return 0;
}

tars::Int32 KVImp::deletePlayerData(tars::Int64 playerId, tars::TarsCurrentPtr current)
{
    if (!_redis) return -1;
    redisReply *reply = (redisReply*)redisCommand(_redis, "DEL %s", playerKey(playerId).c_str());
    if (reply) freeReplyObject(reply);
    return 0;
}

tars::Int32 KVImp::batchGetPlayerData(const vector<tars::Int64>& playerIds, vector<GameDemo::PlayerKVData>& dataList, tars::TarsCurrentPtr current)
{
    TLOG_DEBUG("[KVImp::batchGetPlayerData] count=" << playerIds.size() << endl);
    if (!_redis) {
        TLOG_ERROR("[KVImp::batchGetPlayerData] Redis not connected!" << endl);
        return -1;
    }

    // Pipeline: 先批量发送请求
    for (tars::Int64 playerId : playerIds) {
        redisAppendCommand(_redis, "HGETALL %s", playerKey(playerId).c_str());
    }

    // Pipeline: 批量获取结果
    for (size_t i = 0; i < playerIds.size(); i++) {
        redisReply *reply = NULL;
        if (redisGetReply(_redis, (void**)&reply) != REDIS_OK || !reply) {
            if (reply) freeReplyObject(reply);
            continue;
        }

        if (reply->type != REDIS_REPLY_ARRAY || reply->elements == 0) {
            freeReplyObject(reply);
            continue;
        }

        GameDemo::PlayerKVData data;
        data.playerId = playerIds[i];

        for (size_t j = 0; j < reply->elements; j += 2) {
            string field = reply->element[j]->str;
            string value = reply->element[j + 1]->str;

            if (field == "level") data.level = TC_Common::strto<int>(value);
            else if (field == "x") data.x = TC_Common::strto<float>(value);
            else if (field == "y") data.y = TC_Common::strto<float>(value);
            else if (field == "z") data.z = TC_Common::strto<float>(value);
            else if (field == "sceneId") data.sceneId = TC_Common::strto<int>(value);
            else if (field == "updateTime") data.updateTime = TC_Common::strto<long>(value);
        }

        dataList.push_back(data);
        freeReplyObject(reply);
    }

    TLOG_DEBUG("[KVImp::batchGetPlayerData] done, got=" << dataList.size() << endl);
    return 0;
}
