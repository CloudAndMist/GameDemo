#include <iostream>
#include <string>
#include "servant/Communicator.h"
#include "Lobby.h"

using namespace std;
using namespace tars;
using namespace GameDemo;

int main(int argc, char** argv)
{
    CommunicatorPtr comm = new Communicator();

    try {
        // 设置连接超时时间
        comm->setProperty("connect-timeout", "5000");
        comm->setProperty("recv-timeout", "5000");

        // 通过主控 (tars-registry) 自动查询服务地址
        comm->setProperty("locator", "tars.tarsregistry.QueryObj@tcp -h tars-framework -p 17890");
        cout << "[服务发现] 使用主控: tars-framework:17890" << endl;

        // 获取 LobbyServer 代理
        LobbyServantPrx prx = comm->stringToProxy<LobbyServantPrx>(
            "GameDemo.LobbyServer.LobbyObj"
        );
        cout << "[服务发现] 已通过主控获取 LobbyServer 代理" << endl;

        cout << "========================================" << endl;
        cout << "  Testing LobbyServer RPC" << endl;
        cout << "========================================" << endl;

        // 测试数据
        string qqNumber = "10001";
        string password = "test123";

        // =============================================
        // Test 1: Register Account - 注册账号
        // =============================================
        cout << "\n[1] Testing registerAccount..." << endl;
        cout << "    qqNumber=" << qqNumber << ", password=" << password << endl;

        RegisterReq regReq;
        regReq.qqNumber = qqNumber;
        regReq.password = password;

        RegisterRsp regRsp;
        Int32 ret = prx->registerAccount(regReq, regRsp);

        if (ret == 0 && regRsp.ret == 0) {
            cout << "    [OK] registerAccount success!" << endl;
            cout << "         ret=" << regRsp.ret << endl;
            cout << "         msg=" << regRsp.msg << endl;
            cout << "         accountId=" << regRsp.accountId << endl;
        } else {
            cout << "    [INFO] registerAccount ret=" << ret << ", rsp.ret=" << regRsp.ret << endl;
            cout << "           msg=" << regRsp.msg << endl;
        }

        Int64 accountId = regRsp.accountId;

        // =============================================
        // Test 2: Login - 登录
        // =============================================
        cout << "\n[2] Testing login..." << endl;
        cout << "    qqNumber=" << qqNumber << ", password=" << password << endl;

        LoginReq loginReq;
        loginReq.qqNumber = qqNumber;
        loginReq.password = password;

        LoginRsp loginRsp;
        ret = prx->login(loginReq, loginRsp);

        if (ret == 0 && loginRsp.ret == 0) {
            cout << "    [OK] login success!" << endl;
            cout << "         ret=" << loginRsp.ret << endl;
            cout << "         msg=" << loginRsp.msg << endl;
            cout << "         accountId=" << loginRsp.accountId << endl;
            cout << "         playerId=" << loginRsp.playerId << endl;
            accountId = loginRsp.accountId;
        } else {
            cout << "    [ERROR] login failed, ret=" << ret << ", rsp.ret=" << loginRsp.ret << endl;
            cout << "            msg=" << loginRsp.msg << endl;
            return 1;
        }

        // =============================================
        // Test 3: Get Role List - 获取角色列表
        // =============================================
        cout << "\n[3] Testing getRoleList..." << endl;
        cout << "    accountId=" << accountId << endl;

        GetRoleListReq roleListReq;
        roleListReq.accountId = accountId;

        GetRoleListRsp roleListRsp;
        ret = prx->getRoleList(roleListReq, roleListRsp);

        if (ret == 0 && roleListRsp.ret == 0) {
            cout << "    [OK] getRoleList success!" << endl;
            cout << "         ret=" << roleListRsp.ret << endl;
            cout << "         roles count=" << roleListRsp.roles.size() << endl;
            for (size_t i = 0; i < roleListRsp.roles.size(); ++i) {
                const RoleInfo& role = roleListRsp.roles[i];
                cout << "         Role[" << i << "]: id=" << role.id 
                     << ", name=" << role.roleName 
                     << ", job=" << role.job << endl;
            }
        } else {
            cout << "    [ERROR] getRoleList failed, ret=" << ret << endl;
        }

        Int64 playerId = 0;

        // =============================================
        // Test 4: Create Role - 创建角色 (如果没有角色)
        // =============================================
        if (roleListRsp.roles.empty()) {
            cout << "\n[4] Testing createRole..." << endl;
            cout << "    accountId=" << accountId << ", roleName=TestHero, job=1" << endl;

            CreateRoleReq createReq;
            createReq.accountId = accountId;
            createReq.roleName = "TestHero";
            createReq.job = 1;  // 战士

            CreateRoleRsp createRsp;
            ret = prx->createRole(createReq, createRsp);

            if (ret == 0 && createRsp.ret == 0) {
                cout << "    [OK] createRole success!" << endl;
                cout << "         ret=" << createRsp.ret << endl;
                cout << "         msg=" << createRsp.msg << endl;
                cout << "         playerId=" << createRsp.playerId << endl;
                if (createRsp.role.id > 0) {
                    cout << "         roleId=" << createRsp.role.id << endl;
                    cout << "         roleName=" << createRsp.role.roleName << endl;
                }
                playerId = createRsp.playerId;
            } else {
                cout << "    [ERROR] createRole failed, ret=" << ret << endl;
            }
        } else {
            // 使用已有角色
            playerId = roleListRsp.roles[0].id;
            cout << "\n[4] Using existing role, playerId=" << playerId << endl;
        }

        // =============================================
        // Test 5: Select Role - 选择角色
        // =============================================
        if (playerId > 0) {
            cout << "\n[5] Testing selectRole..." << endl;
            cout << "    accountId=" << accountId << ", playerId=" << playerId << endl;

            SelectRoleReq selectReq;
            selectReq.accountId = accountId;
            selectReq.roleId = playerId;

            SelectRoleRsp selectRsp;
            ret = prx->selectRole(selectReq, selectRsp);

            if (ret == 0 && selectRsp.ret == 0) {
                cout << "    [OK] selectRole success!" << endl;
                cout << "         ret=" << selectRsp.ret << endl;
                cout << "         playerId=" << selectRsp.playerId << endl;
                if (selectRsp.role.id > 0) {
                    cout << "         roleName=" << selectRsp.role.roleName << endl;
                }
            } else {
                cout << "    [ERROR] selectRole failed, ret=" << ret << endl;
            }
        }

        // =============================================
        // Test 6: Heartbeat - 心跳
        // =============================================
        cout << "\n[6] Testing heartbeat..." << endl;
        cout << "    playerId=" << playerId << endl;

        HeartBeatReq hbReq;
        hbReq.playerId = playerId;

        ret = prx->heartbeat(hbReq);

        if (ret == 0) {
            cout << "    [OK] heartbeat success!" << endl;
        } else {
            cout << "    [ERROR] heartbeat failed, ret=" << ret << endl;
        }

        // =============================================
        // Test 7: Enter Scene - 进入场景 (转发给 SceneServer)
        // =============================================
        if (playerId > 0) {
            cout << "\n[7] Testing enterScene (via LobbyServer)..." << endl;
            cout << "    playerId=" << playerId << ", sceneId=1" << endl;

            EnterSceneRsp enterRsp;
            ret = prx->enterScene(playerId, 1, enterRsp);

            if (ret == 0 && enterRsp.ret == 0) {
                cout << "    [OK] enterScene success!" << endl;
                cout << "         ret=" << enterRsp.ret << endl;
                cout << "         msg=" << enterRsp.msg << endl;
                cout << "         self: playerId=" << enterRsp.self.playerId 
                     << ", pos=(" << enterRsp.self.x << "," << enterRsp.self.y << "," << enterRsp.self.z << ")" << endl;
                cout << "         other players in scene: " << enterRsp.players.size() << endl;
            } else {
                cout << "    [ERROR] enterScene failed, ret=" << ret << ", rsp.ret=" << enterRsp.ret << endl;
            }

            // =============================================
            // Test 8: Move - 移动 (转发给 SceneServer)
            // =============================================
            cout << "\n[8] Testing move (via LobbyServer)..." << endl;

            MoveReq moveReq;
            moveReq.playerId = playerId;
            moveReq.x = 100.0f;
            moveReq.y = 200.0f;
            moveReq.z = 300.0f;

            MoveRsp moveRsp;
            ret = prx->move(moveReq, moveRsp);

            if (ret == 0 && moveRsp.ret == 0) {
                cout << "    [OK] move success!" << endl;
                cout << "         ret=" << moveRsp.ret << endl;
                cout << "         msg=" << moveRsp.msg << endl;
            } else {
                cout << "    [ERROR] move failed, ret=" << ret << endl;
            }

            // =============================================
            // Test 9: Leave Scene - 离开场景 (转发给 SceneServer)
            // =============================================
            cout << "\n[9] Testing leaveScene (via LobbyServer)..." << endl;

            LeaveSceneRsp leaveRsp;
            ret = prx->leaveScene(playerId, 1, leaveRsp);

            if (ret == 0 && leaveRsp.ret == 0) {
                cout << "    [OK] leaveScene success!" << endl;
                cout << "         ret=" << leaveRsp.ret << endl;
                cout << "         msg=" << leaveRsp.msg << endl;
            } else {
                cout << "    [ERROR] leaveScene failed, ret=" << ret << endl;
            }
        }

        cout << "\n========================================" << endl;
        cout << "  All tests completed!" << endl;
        cout << "========================================" << endl;

    } catch (exception& e) {
        cerr << "[EXCEPTION] " << e.what() << endl;
        return 1;
    }

    return 0;
}
