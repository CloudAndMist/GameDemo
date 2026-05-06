#include <iostream>
#include "servant/Communicator.h"
#include "DB.h"

using namespace std;
using namespace tars;
using namespace GameDemo;

int main(int argc, char** argv)
{
    CommunicatorPtr comm = new Communicator();
    
    try {
        // 设置连接超时时间
        comm->setProperty("connect-timeout", "3000");
        
        // 获取 DBServer KV 代理
        KVServantPrx prx = comm->stringToProxy<KVServantPrx>("GameDemo.DBServer.KVObj@tcp -h 172.25.0.5 -p 10004");
        
        cout << "=== Testing DBServer V0.5 Redis KV RPC interfaces ===" << endl;
        
        // Test 1: savePlayerData
        cout << "\n[1] Testing savePlayerData(playerId: 1)..." << endl;
        PlayerKVData playerData;
        playerData.playerId = 1;
        playerData.level = 10;
        playerData.x = 100.5f;
        playerData.y = 0.0f;
        playerData.z = 200.3f;
        playerData.sceneId = 1001;
        playerData.updateTime = 0;
        
        tars::Int32 ret = prx->savePlayerData(playerData);
        cout << "    Result: ret=" << ret << (ret == 0 ? " (OK)" : " (Error)") << endl;
        
        // Test 2: getPlayerData
        cout << "\n[2] Testing getPlayerData(playerId: 1)..." << endl;
        PlayerKVData getData;
        ret = prx->getPlayerData(1, getData);
        if (ret == 0) {
            cout << "    Result: OK" << endl;
            cout << "    - playerId: " << getData.playerId << endl;
            cout << "    - level: " << getData.level << endl;
            cout << "    - position: (" << getData.x << ", " << getData.y << ", " << getData.z << ")" << endl;
            cout << "    - sceneId: " << getData.sceneId << endl;
        } else {
            cout << "    Result: Error " << ret << endl;
        }
        
        // Test 3: updatePlayerData (调用 savePlayerData 更新)
        cout << "\n[3] Testing updatePlayerData (level up to 15, move to new position)..." << endl;
        getData.level = 15;
        getData.x = 300.0f;
        getData.z = 400.0f;
        getData.sceneId = 1002;
        ret = prx->savePlayerData(getData);
        cout << "    Result: ret=" << ret << endl;
        
        // Test 4: getPlayerData (verify update)
        cout << "\n[4] Testing getPlayerData (verify update)..." << endl;
        PlayerKVData verifyData;
        ret = prx->getPlayerData(1, verifyData);
        if (ret == 0) {
            cout << "    Result: OK" << endl;
            cout << "    - level: " << verifyData.level << " (expected: 15)" << endl;
            cout << "    - position: (" << verifyData.x << ", " << verifyData.z << ") (expected: 300, 400)" << endl;
            cout << "    - sceneId: " << verifyData.sceneId << " (expected: 1002)" << endl;
        } else {
            cout << "    Result: Error " << ret << endl;
        }
        
        // Test 5: batchSavePlayerData
        cout << "\n[5] Testing batchSavePlayerData (playerId: 2, 3, 4)..." << endl;
        vector<PlayerKVData> batchData;
        for (int i = 2; i <= 4; i++) {
            PlayerKVData data;
            data.playerId = i;
            data.level = i * 5;
            data.x = i * 10.0f;
            data.y = 0.0f;
            data.z = i * 20.0f;
            data.sceneId = 1000 + i;
            batchData.push_back(data);
        }
        ret = prx->batchSavePlayerData(batchData);
        cout << "    Result: ret=" << ret << " (saved " << batchData.size() << " players)" << endl;
        
        // Test 6: batchGetPlayerData
        cout << "\n[6] Testing batchGetPlayerData (playerId: 2, 3, 4)..." << endl;
        vector<long> playerIds;
        playerIds.push_back(2);
        playerIds.push_back(3);
        playerIds.push_back(4);
        vector<PlayerKVData> getBatchData;
        ret = prx->batchGetPlayerData(playerIds, getBatchData);
        if (ret == 0) {
            cout << "    Result: OK, got " << getBatchData.size() << " players" << endl;
            for (size_t i = 0; i < getBatchData.size(); i++) {
                cout << "    - Player " << getBatchData[i].playerId 
                     << ": level=" << getBatchData[i].level 
                     << ", scene=" << getBatchData[i].sceneId << endl;
            }
        } else {
            cout << "    Result: Error " << ret << endl;
        }
        
        // Test 7: deletePlayerData
        cout << "\n[7] Testing deletePlayerData(playerId: 1)..." << endl;
        ret = prx->deletePlayerData(1);
        cout << "    Result: ret=" << ret << endl;
        
        // Test 8: getPlayerData (verify delete)
        cout << "\n[8] Testing getPlayerData (verify delete, should fail)..." << endl;
        PlayerKVData deletedData;
        ret = prx->getPlayerData(1, deletedData);
        cout << "    Result: ret=" << ret 
             << (ret == ERR_KV_NOT_FOUND ? " (ERR_KV_NOT_FOUND - expected)" : " (unexpected)")
             << endl;
        
        cout << "\n=== All KV tests completed! ===" << endl;
    }
    catch (exception& e) {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }
    
    return 0;
}
