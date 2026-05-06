#pragma once

#include "DB.h"
#include <hiredis/hiredis.h>
#include <string>

class KVImp : public GameDemo::KVServant
{
public:
    void initialize() override;
    void destroy() override;

    tars::Int32 batchGetPlayerData(const vector<tars::Int64>& playerIds,
                                   vector<GameDemo::PlayerKVData>& dataList,
                                   tars::TarsCurrentPtr current) override;

    tars::Int32 batchSavePlayerData(const vector<GameDemo::PlayerKVData>& dataList,
                                    tars::TarsCurrentPtr current) override;

    tars::Int32 deletePlayerData(tars::Int64 playerId,
                                 tars::TarsCurrentPtr current) override;

    tars::Int32 getPlayerData(tars::Int64 playerId,
                              GameDemo::PlayerKVData& data,
                              tars::TarsCurrentPtr current) override;

    tars::Int32 savePlayerData(const GameDemo::PlayerKVData& data,
                               tars::TarsCurrentPtr current) override;

private:
    bool connectRedis();
    std::string playerKey(tars::Int64 playerId);

private:
    redisContext* _redis;
    std::string _redisHost;
    int _redisPort;
    int _dataTTL;
};
