#include <iostream>
#include <string>
#include "servant/Communicator.h"
#include "Scene.h"

using namespace std;
using namespace tars;
using namespace GameDemo;

int main(int argc, char** argv)
{
    CommunicatorPtr comm = new Communicator();

    try {
        // 设置连接超时时间
        comm->setProperty("connect-timeout", "3000");

        // ============================================
        // 方式1: 使用服务发现 (推荐)
        // 通过主控 (tars-registry) 自动查询服务地址
        // ============================================
        comm->setProperty("locator", "tars.tarsregistry.QueryObj@tcp -h tars-framework -p 17890");
        cout << "[服务发现] 使用主控: tars-framework:17890" << endl;

        // 获取 SceneServer 代理 (模拟 LobbyServer 调用)
        // 格式: 应用名.服务名. servant 对象名
        SceneServantPrx prx = comm->stringToProxy<SceneServantPrx>(
            "GameDemo.SceneServer.SceneObj"
        );
        cout << "[服务发现] 已通过主控获取 SceneServer 代理" << endl;

        cout << "========================================" << endl;
        cout << "  Testing SceneServer RPC (模拟 LobbyServer)" << endl;
        cout << "========================================" << endl;

        // 测试数据
        tars::Int64 playerId = 1001;
        tars::Int32 sceneId = 1;

        // =============================================
        // Test 1: enterScene - 玩家进入场景
        // =============================================
        cout << "\n[1] Testing enterScene..." << endl;
        cout << "    playerId=" << playerId << ", sceneId=" << sceneId << endl;

        EnterSceneReq enterReq;
        enterReq.playerId = playerId;
        enterReq.sceneId = sceneId;

        EnterSceneRsp enterRsp;
        tars::Int32 ret = prx->enterScene(enterReq, enterRsp);

        if (ret == 0) {
            cout << "    [OK] enterScene success!" << endl;
            cout << "         ret=" << enterRsp.ret << endl;
            cout << "         msg=" << enterRsp.msg << endl;
            cout << "         sceneId=" << enterRsp.self.sceneId << endl;
            cout << "         playerId=" << enterRsp.self.playerId << endl;
            cout << "         players in scene=" << enterRsp.players.size() << endl;
        } else {
            cout << "    [ERROR] enterScene failed, ret=" << ret << endl;
        }

        // =============================================
        // Test 2: move - 玩家移动
        // =============================================
        cout << "\n[2] Testing move..." << endl;
        cout << "    playerId=" << playerId << endl;

        MoveReq moveReq;
        moveReq.playerId = playerId;
        moveReq.x = 100.5f;
        moveReq.y = 200.5f;
        moveReq.z = 300.5f;

        MoveRsp moveRsp;
        ret = prx->move(moveReq, moveRsp);

        if (ret == 0) {
            cout << "    [OK] move success!" << endl;
            cout << "         ret=" << moveRsp.ret << endl;
            cout << "         msg=" << moveRsp.msg << endl;
        } else {
            cout << "    [ERROR] move failed, ret=" << ret << endl;
        }

        // =============================================
        // Test 3: heartbeat - 心跳保活
        // =============================================
        cout << "\n[3] Testing heartbeat..." << endl;
        cout << "    playerId=" << playerId << endl;

        HeartBeatReq heartReq;
        heartReq.playerId = playerId;

        ret = prx->heartbeat(heartReq);

        if (ret == 0) {
            cout << "    [OK] heartbeat success!" << endl;
        } else {
            cout << "    [ERROR] heartbeat failed, ret=" << ret << endl;
        }

        // =============================================
        // Test 4: getScenePlayers - 获取场景玩家列表
        // =============================================
        cout << "\n[4] Testing getScenePlayers..." << endl;
        cout << "    playerId=" << playerId << ", sceneId=" << sceneId << endl;

        GetScenePlayersRsp playersRsp;
        ret = prx->getScenePlayers(playerId, sceneId, playersRsp);

        if (ret == 0) {
            cout << "    [OK] getScenePlayers success!" << endl;
            cout << "         ret=" << playersRsp.ret << endl;
            cout << "         msg=" << playersRsp.msg << endl;
            cout << "         players count=" << playersRsp.players.size() << endl;
            for (size_t i = 0; i < playersRsp.players.size(); ++i) {
                const PlayerInfo& p = playersRsp.players[i];
                cout << "         Player[" << i << "]: id=" << p.playerId
                     << ", name=" << p.roleName
                     << ", level=" << p.level
                     << ", pos=(" << p.x << ", " << p.y << ", " << p.z << ")"
                     << endl;
            }
        } else {
            cout << "    [ERROR] getScenePlayers failed, ret=" << ret << endl;
        }

        // =============================================
        // Test 5: 多玩家进入场景测试
        // =============================================
        cout << "\n[5] Testing multi-player enterScene..." << endl;

        for (int i = 2; i <= 4; ++i) {
            tars::Int64 newPlayerId = 1000 + i;
            EnterSceneReq req;
            req.playerId = newPlayerId;
            req.sceneId = sceneId;

            EnterSceneRsp rsp;
            ret = prx->enterScene(req, rsp);

            if (ret == 0) {
                cout << "    [OK] Player" << i << " (id=" << newPlayerId << ") entered scene" << endl;
            } else {
                cout << "    [ERROR] Player" << i << " failed, ret=" << ret << endl;
            }
        }

        // 再次获取玩家列表验证
        cout << "\n[5b] getScenePlayers after multi-player enter..." << endl;
        ret = prx->getScenePlayers(playerId, sceneId, playersRsp);
        if (ret == 0) {
            cout << "    Players in scene " << sceneId << ": " << playersRsp.players.size() << endl;
            for (size_t i = 0; i < playersRsp.players.size(); ++i) {
                cout << "         Player[" << i << "]: id=" << playersRsp.players[i].playerId << endl;
            }
        }

        // =============================================
        // Test 6: leaveScene - 玩家离开场景
        // =============================================
        cout << "\n[6] Testing leaveScene..." << endl;
        cout << "    playerId=" << playerId << ", sceneId=" << sceneId << endl;

        LeaveSceneReq leaveReq;
        leaveReq.playerId = playerId;
        leaveReq.sceneId = sceneId;

        LeaveSceneRsp leaveRsp;
        ret = prx->leaveScene(leaveReq, leaveRsp);

        if (ret == 0) {
            cout << "    [OK] leaveScene success!" << endl;
            cout << "         ret=" << leaveRsp.ret << endl;
            cout << "         msg=" << leaveRsp.msg << endl;
        } else {
            cout << "    [ERROR] leaveScene failed, ret=" << ret << endl;
        }

        // 验证玩家已离开
        cout << "\n[6b] getScenePlayers after leave..." << endl;
        GetScenePlayersRsp afterLeaveRsp;
        ret = prx->getScenePlayers(playerId, sceneId, afterLeaveRsp);
        if (ret == 0) {
            cout << "    Players in scene " << sceneId << " after leave: " << afterLeaveRsp.players.size() << endl;
            // 注意：此时 playerId 1001 应该不在列表中了
            bool found = false;
            for (size_t i = 0; i < afterLeaveRsp.players.size(); ++i) {
                if (afterLeaveRsp.players[i].playerId == playerId) {
                    found = true;
                    break;
                }
            }
            cout << "    Player " << playerId << " still in scene: " << (found ? "YES [ERROR]" : "NO [OK]") << endl;
        }

        cout << "\n========================================" << endl;
        cout << "  All tests completed!" << endl;
        cout << "========================================" << endl;
    }
    catch (exception& e) {
        cerr << "\n[EXCEPTION] " << e.what() << endl;
        return -1;
    }

    return 0;
}
