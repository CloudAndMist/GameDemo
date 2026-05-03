/**
 * TestGameClientV0.2 - V0.2 模块测试客户端
 * 
 * 支持测试功能：
 * 1. 账号注册/登录
 * 2. 角色创建/列表/选择
 * 3. 进入场景/移动/离开场景
 * 4. 心跳机制
 * 
 * 编译: 在 tars-cpp-compiler 容器中
 *   cd /workspace/client/TestGameClientV0.2
 *   mkdir -p build && cd build
 *   cmake .. && make
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <sstream>
#include <functional>
#include <cstdlib>
#include <ctime>

#include "GameDemoBase.h"
#include "Lobby.h"
#include "Push.h"
#include "servant/Communicator.h"

using namespace std;
using namespace GameDemo;

// 颜色输出
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

#define LOG_OK(msg)    cout << COLOR_GREEN << "[OK] " << COLOR_RESET << msg << endl
#define LOG_INFO(msg)  cout << COLOR_CYAN << "[INFO] " << COLOR_RESET << msg << endl
#define LOG_WARN(msg)  cout << COLOR_YELLOW << "[WARN] " << COLOR_RESET << msg << endl
#define LOG_ERR(msg)   cout << COLOR_GREEN << "[ERROR] " << COLOR_RESET << msg << endl

// ========== PushCallback 实现 ==========
// 客户端接收服务端推送的回调类
class LobbyPushCallback : public GameDemo::Lobby2ClientPushPrxCallback
{
public:
    LobbyPushCallback() {}
    virtual ~LobbyPushCallback() {}

    // 玩家进入场景推送
    virtual void callback_onPlayerEnter(tars::Int32 ret, const GameDemo::PlayerEnterNotify& notify)
    {
        cout << COLOR_CYAN << "[PUSH] " << COLOR_RESET;
        cout << "玩家进入: " << notify.player.roleName;
        cout << " (ID=" << notify.player.playerId << ")";
        cout << " 位置: (" << notify.player.x << ", " << notify.player.y << ", " << notify.player.z << ")" << endl;
    }

    // 玩家移动推送
    virtual void callback_onPlayerMove(tars::Int32 ret, const GameDemo::PlayerMoveNotify& notify)
    {
        cout << COLOR_CYAN << "[PUSH] " << COLOR_RESET;
        cout << "玩家移动: playerId=" << notify.playerId;
        cout << " 位置: (" << notify.x << ", " << notify.y << ", " << notify.z << ")" << endl;
    }

    // 玩家离开场景推送
    virtual void callback_onPlayerLeave(tars::Int32 ret, const GameDemo::PlayerLeaveNotify& notify)
    {
        cout << COLOR_YELLOW << "[PUSH] " << COLOR_RESET;
        cout << "玩家离开: playerId=" << notify.playerId << endl;
    }
};

// 错误码说明
string getErrorMsg(int ret) {
    switch (ret) {
        case 0:    return "成功";
        case 1001: return "账号已存在";
        case 1002: return "账号不存在";
        case 1003: return "密码错误";
        case 2001: return "角色已存在";
        case 2002: return "角色不存在";
        case 5001: return "数据库错误";
        case 9999: return "服务器繁忙";
        default:   return "未知错误";
    }
}

class TestGameClientV0_2 {
public:
    TestGameClientV0_2()
        : _comm(new tars::Communicator())
        , _pushCallback(nullptr)
        , _accountId(0)
        , _playerId(0)
        , _sceneId(0)
        , _isLogin(false)
        , _inScene(false)
        , _running(true)
        , _heartbeatThread(nullptr)
    {
        // 设置 Tars 框架定位器
        _comm->setProperty("locator", "tars.tarsregistry.QueryObj@tcp -h tars-framework -p 17890");
        
        // 创建 LobbyServer 代理
        _lobbyPrx = _comm->stringToProxy<LobbyServantPrx>("GameDemo.LobbyServer.LobbyObj");
        
        // 初始化随机数种子
        srand(static_cast<unsigned>(time(nullptr)));

        // 注册 Push 回调
        registerPushCallback();
    }

    // 注册 Push 回调
    void registerPushCallback() {
        try {
            _pushCallback = new LobbyPushCallback();
            _lobbyPrx->tars_set_push_callback(_pushCallback);
            LOG_INFO("Push 回调注册成功");
        } catch (const std::exception& e) {
            LOG_WARN("Push 回调注册失败: " << string(e.what()));
        }
    }
    
    ~TestGameClientV0_2() {
        cleanup();
    }
    
    void cleanup() {
        _running = false;
        if (_heartbeatThread && _heartbeatThread->joinable()) {
            _heartbeatThread->join();
        }
    }
    
    // ==================== 账号模块 ====================
    
    bool registerAccount(const string& qqNumber, const string& password) {
        cout << "\n--- [注册账号] ---" << endl;
        cout << "qqNumber: " << qqNumber << endl;
        cout << "password: " << password << endl;
        
        try {
            RegisterReq req;
            req.qqNumber = qqNumber;
            req.password = password;
            RegisterRsp rsp;
            
            tars::Int32 ret = _lobbyPrx->registerAccount(req, rsp);
            
            if (ret == 0) {
                LOG_OK("注册成功! accountId=" << rsp.accountId);
                return true;
            } else {
                LOG_ERR("注册失败: ret=" << ret << " (" << getErrorMsg(ret) << ")");
                return false;
            }
        } catch (const std::exception& e) {
            LOG_ERR("注册异常: " << e.what());
            return false;
        }
    }
    
    bool login(const string& qqNumber, const string& password) {
        cout << "\n--- [登录] ---" << endl;
        cout << "qqNumber: " << qqNumber << endl;
        cout << "password: " << string(password.length(), '*') << endl;
        
        try {
            LoginReq req;
            req.qqNumber = qqNumber;
            req.password = password;
            LoginRsp rsp;
            
            tars::Int32 ret = _lobbyPrx->login(req, rsp);
            
            if (ret == 0) {
                _accountId = rsp.accountId;
                _playerId = rsp.playerId;  // 注意：login返回的playerId可能为0
                _isLogin = true;
                LOG_OK("登录成功!");
                // 注意：registerPush 应在 selectRole 之后调用
                cout << "  accountId: " << _accountId << endl;
                cout << "  playerId: " << _playerId << " (请在 selectRole 后使用 registerpush 命令)" << endl;
                return true;
            } else {
                LOG_ERR("登录失败: ret=" << ret << " (" << getErrorMsg(ret) << ")");
                return false;
            }
        } catch (const std::exception& e) {
            LOG_ERR("登录异常: " << e.what());
            return false;
        }
    }
    
    void logout() {
        if (!_isLogin) return;
        cout << "\n--- [登出] ---" << endl;
        LOG_INFO("登出账号: accountId=" << _accountId);
        _isLogin = false;
        _inScene = false;
        _accountId = 0;
        _playerId = 0;
        _sceneId = 0;
        stopHeartbeat();
    }
    
    // ==================== 角色模块 ====================
    
    bool getRoleList() {
        if (!_isLogin) {
            LOG_ERR("请先登录!");
            return false;
        }
        
        cout << "\n--- [获取角色列表] ---" << endl;
        cout << "accountId: " << _accountId << endl;
        
        try {
            GetRoleListReq req;
            req.accountId = _accountId;
            GetRoleListRsp rsp;
            
            tars::Int32 ret = _lobbyPrx->getRoleList(req, rsp);
            
            if (ret == 0) {
                LOG_OK("获取成功! 角色数量=" << rsp.roles.size());
                cout << endl;
                cout << setw(10) << "ID" << setw(15) << "名称" << setw(8) << "职业" 
                     << setw(8) << "等级" << setw(8) << "HP" << setw(8) << "MP" << endl;
                cout << string(60, '-') << endl;
                
                for (const auto& role : rsp.roles) {
                    string jobName;
                    switch (role.job) {
                        case 1: jobName = "战士"; break;
                        case 2: jobName = "法师"; break;
                        case 3: jobName = "猎人"; break;
                        default: jobName = "未知"; break;
                    }
                    cout << setw(10) << role.id 
                         << setw(15) << role.roleName 
                         << setw(8) << jobName
                         << setw(8) << role.level 
                         << setw(8) << role.hp 
                         << setw(8) << role.mp 
                         << endl;
                }
                return true;
            } else {
                LOG_ERR("获取失败: ret=" << ret << " (" << getErrorMsg(ret) << ")");
                return false;
            }
        } catch (const std::exception& e) {
            LOG_ERR("获取异常: " << e.what());
            return false;
        }
    }
    
    bool createRole(const string& roleName, int job) {
        if (!_isLogin) {
            LOG_ERR("请先登录!");
            return false;
        }
        
        cout << "\n--- [创建角色] ---" << endl;
        cout << "accountId: " << _accountId << endl;
        cout << "roleName: " << roleName << endl;
        
        string jobName;
        switch (job) {
            case 1: jobName = "战士"; break;
            case 2: jobName = "法师"; break;
            case 3: jobName = "猎人"; break;
            default: jobName = "未知"; break;
        }
        cout << "job: " << job << " (" << jobName << ")" << endl;
        
        try {
            CreateRoleReq req;
            req.accountId = _accountId;
            req.roleName = roleName;
            req.job = job;
            CreateRoleRsp rsp;
            
            tars::Int32 ret = _lobbyPrx->createRole(req, rsp);
            
            if (ret == 0) {
                LOG_OK("创建成功!");
                cout << "  playerId: " << rsp.playerId << endl;
                cout << "  roleId: " << rsp.role.id << endl;
                cout << "  roleName: " << rsp.role.roleName << endl;
                return true;
            } else {
                LOG_ERR("创建失败: ret=" << ret << " (" << getErrorMsg(ret) << ")");
                return false;
            }
        } catch (const std::exception& e) {
            LOG_ERR("创建异常: " << e.what());
            return false;
        }
    }
    
    bool selectRole(long roleId) {
        if (!_isLogin) {
            LOG_ERR("请先登录!");
            return false;
        }
        
        cout << "\n--- [选择角色] ---" << endl;
        cout << "accountId: " << _accountId << endl;
        cout << "roleId: " << roleId << endl;
        
        try {
            SelectRoleReq req;
            req.accountId = _accountId;
            req.roleId = roleId;
            SelectRoleRsp rsp;
            
            tars::Int32 ret = _lobbyPrx->selectRole(req, rsp);
            
            if (ret == 0) {
                _playerId = rsp.playerId;
                LOG_OK("选择成功!");
                cout << "  playerId: " << _playerId << endl;
                cout << "  roleName: " << rsp.role.roleName << endl;

                // 选择角色后注册推送回调
                if (_playerId > 0) {
                    try {
                        _lobbyPrx->registerPush(_playerId);
                        LOG_INFO("推送回调注册成功!");
                    } catch (const std::exception& e) {
                        LOG_WARN("推送回调注册失败: " << e.what());
                    }
                }
                return true;
            } else {
                LOG_ERR("选择失败: ret=" << ret << " (" << getErrorMsg(ret) << ")");
                return false;
            }
        } catch (const std::exception& e) {
            LOG_ERR("选择异常: " << e.what());
            return false;
        }
    }
    
    // ==================== 场景模块 ====================
    
    bool enterScene(int sceneId = 1) {
        if (!_isLogin || _playerId == 0) {
            LOG_ERR("请先登录并选择角色!");
            return false;
        }
        
        cout << "\n--- [进入场景] ---" << endl;
        cout << "playerId: " << _playerId << endl;
        cout << "sceneId: " << sceneId << endl;
        
        try {
            EnterSceneRsp rsp;
            tars::Int32 ret = _lobbyPrx->enterScene(_playerId, sceneId, rsp);
            
            if (ret == 0) {
                _inScene = true;
                _sceneId = sceneId;
                LOG_OK("进入场景成功!");
                cout << "  场景ID: " << sceneId << endl;
                cout << "  自身位置: (" << rsp.self.x << ", " << rsp.self.y << ", " << rsp.self.z << ")" << endl;
                cout << "  场景内其他玩家: " << rsp.players.size() << endl;
                
                if (!rsp.players.empty()) {
                    cout << endl << "  其他玩家列表:" << endl;
                    cout << setw(10) << "playerId" << setw(15) << "名称" << setw(10) << "等级" 
                         << setw(15) << "位置" << endl;
                    cout << string(55, '-') << endl;
                    for (const auto& p : rsp.players) {
                        cout << setw(10) << p.playerId 
                             << setw(15) << p.roleName 
                             << setw(10) << p.level 
                             << setw(15) << "(" << p.x << "," << p.y << "," << p.z << ")"
                             << endl;
                    }
                }
                return true;
            } else {
                LOG_ERR("进入场景失败: ret=" << ret << " (" << getErrorMsg(ret) << ")");
                return false;
            }
        } catch (const std::exception& e) {
            LOG_ERR("进入场景异常: " << e.what());
            return false;
        }
    }
    
    bool move(float x, float y, float z) {
        if (!_inScene) {
            LOG_ERR("请先进入场景!");
            return false;
        }
        
        cout << "\n--- [移动] ---" << endl;
        cout << "playerId: " << _playerId << endl;
        cout << "目标位置: (" << x << ", " << y << ", " << z << ")" << endl;
        
        try {
            MoveReq req;
            req.playerId = _playerId;
            req.x = x;
            req.y = y;
            req.z = z;
            MoveRsp rsp;
            
            tars::Int32 ret = _lobbyPrx->move(req, rsp);
            
            if (ret == 0) {
                LOG_OK("移动成功!");
                return true;
            } else {
                LOG_ERR("移动失败: ret=" << ret << " (" << getErrorMsg(ret) << ")");
                return false;
            }
        } catch (const std::exception& e) {
            LOG_ERR("移动异常: " << e.what());
            return false;
        }
    }
    
    bool leaveScene() {
        if (!_inScene) {
            LOG_WARN("不在场景中!");
            return false;
        }
        
        cout << "\n--- [离开场景] ---" << endl;
        cout << "playerId: " << _playerId << endl;
        
        try {
            LeaveSceneRsp rsp;
            tars::Int32 ret = _lobbyPrx->leaveScene(_playerId, _sceneId, rsp);
            
            if (ret == 0) {
                _inScene = false;
                LOG_OK("离开场景成功!");
                return true;
            } else {
                LOG_ERR("离开场景失败: ret=" << ret << " (" << getErrorMsg(ret) << ")");
                return false;
            }
        } catch (const std::exception& e) {
            LOG_ERR("离开场景异常: " << e.what());
            return false;
        }
    }
    
    // ==================== 心跳模块 ====================
    
    void startHeartbeat(int intervalSeconds = 10) {
        if (_heartbeatThread) {
            LOG_WARN("心跳已在运行!");
            return;
        }
        
        LOG_INFO("启动心跳... (间隔 " << intervalSeconds << " 秒)");
        _heartbeatThread = new thread([this, intervalSeconds]() {
            while (_running) {
                this_thread::sleep_for(chrono::seconds(intervalSeconds));
                if (!_running) break;
                sendHeartbeat();
            }
        });
    }
    
    void stopHeartbeat() {
        if (_heartbeatThread) {
            LOG_INFO("停止心跳...");
            _running = false;
            if (_heartbeatThread->joinable()) {
                _heartbeatThread->join();
            }
            delete _heartbeatThread;
            _heartbeatThread = nullptr;
            _running = true;
        }
    }
    
    bool sendHeartbeat() {
        if (!_isLogin || _playerId == 0) {
            return false;
        }
        
        try {
            HeartBeatReq req;
            req.playerId = _playerId;
            tars::Int32 ret = _lobbyPrx->heartbeat(req);
            
            if (ret == 0) {
                cout << COLOR_CYAN << "[HEARTBEAT] " << COLOR_RESET;
                cout << "playerId=" << _playerId << " OK" << endl;
                return true;
            } else {
                cout << COLOR_YELLOW << "[HEARTBEAT] " << COLOR_RESET;
                cout << "playerId=" << _playerId << " FAILED, ret=" << ret << endl;
                return false;
            }
        } catch (const std::exception& e) {
            cout << COLOR_YELLOW << "[HEARTBEAT] " << COLOR_RESET;
            cout << "playerId=" << _playerId << " EXCEPTION: " << e.what() << endl;
            return false;
        }
    }
    
    // ==================== 状态查询 ====================
    
    void printStatus() {
        cout << "\n" << string(40, '=') << endl;
        cout << "           当前状态" << endl;
        cout << string(40, '=') << endl;
        cout << "  登录状态: " << (_isLogin ? "已登录" : "未登录") << endl;
        if (_isLogin) {
            cout << "  账号ID: " << _accountId << endl;
            cout << "  玩家ID: " << _playerId << endl;
            cout << "  场景状态: " << (_inScene ? "场景中" : "大厅") << endl;
            if (_inScene) {
                cout << "  场景ID: " << _sceneId << endl;
            }
            cout << "  心跳状态: " << (_heartbeatThread ? "运行中" : "未启动") << endl;
        }
        cout << string(40, '=') << endl;
    }
    
    // ==================== 交互式菜单 ====================
    
    void printHelp() {
        cout << "\n" << COLOR_BOLD;
        cout << "============================================================" << endl;
        cout << "           TestGameClientV0.2 - V0.2 模块测试客户端        " << endl;
        cout << "============================================================" << endl;
        cout << "  [账号模块]                                                  " << endl;
        cout << "    1. register <qq> <pwd>  - 注册账号                       " << endl;
        cout << "    2. login <qq> <pwd>      - 登录                         " << endl;
        cout << "    3. registerpush         - 注册推送 (登录后调用)          " << endl;
        cout << "    4. logout                - 登出                         " << endl;
        cout << "                                                              " << endl;
        cout << "  [角色模块]                                                  " << endl;
        cout << "    4. rolelist             - 获取角色列表                   " << endl;
        cout << "    5. createrole <name> <job> - 创建角色 (1战士/2法师/3猎人) " << endl;
        cout << "    6. selectrole <roleId>  - 选择角色                       " << endl;
        cout << "                                                              " << endl;
        cout << "  [场景模块]                                                  " << endl;
        cout << "    7. enter [sceneId]      - 进入场景 (默认sceneId=1)        " << endl;
        cout << "    8. move <x> <y> <z>    - 移动                          " << endl;
        cout << "    9. randommove           - 随机移动 (后台)                " << endl;
        cout << "   10. leavescene           - 离开场景                     " << endl;
        cout << "                                                              " << endl;
        cout << "  [心跳模块]                                                  " << endl;
        cout << "   11. heartbeat            - 发送一次心跳                   " << endl;
        cout << "   12. starthb [sec]       - 启动心跳 (默认10秒间隔)       " << endl;
        cout << "   13. stophb               - 停止心跳                      " << endl;
        cout << "                                                              " << endl;
        cout << "  [系统]                                                      " << endl;
        cout << "   14. status               - 查看当前状态                   " << endl;
        cout << "   15. help                 - 显示帮助                      " << endl;
        cout << "   16. quit                 - 退出程序                      " << endl;
        cout << "============================================================" << endl;
        cout << COLOR_RESET << endl;
    }
    
    // ==================== 主循环 ====================
    
    void run() {
        cout << COLOR_BOLD;
        cout << "\n============================================================" << endl;
        cout << "           TestGameClientV0.2 - V0.2 模块测试客户端        " << endl;
        cout << "                                                              " << endl;
        cout << "  V0.2 测试范围:                                              " << endl;
        cout << "    - 账号注册/登录                                           " << endl;
        cout << "    - 角色创建/列表/选择                                       " << endl;
        cout << "    - 进入场景/移动/离开场景                                   " << endl;
        cout << "    - 心跳机制                                                " << endl;
        cout << "============================================================" << endl;
        cout << COLOR_RESET << endl;
        
        LOG_INFO("连接 LobbyServer: GameDemo.LobbyServer.LobbyObj");
        printHelp();
        
        string cmd;
        while (_running && cout << "\n> ", getline(cin, cmd)) {
            if (cmd.empty()) continue;
            
            // 解析命令
            istringstream iss(cmd);
            string op;
            iss >> op;
            
            // ==================== 退出 ====================
            if (op == "quit" || op == "exit") {
                cleanup();
                cout << "再见!" << endl;
                break;
            }
            
            // ==================== 帮助 ====================
            else if (op == "help" || op == "?") {
                printHelp();
            }
            
            // ==================== 状态 ====================
            else if (op == "status" || op == "stat") {
                printStatus();
            }
            
            // ==================== 账号模块 ====================
            else if (op == "register" || op == "reg") {
                string qq, pwd;
                iss >> qq >> pwd;
                if (qq.empty() || pwd.empty()) {
                    LOG_ERR("用法: register <qq号> <密码>");
                } else {
                    registerAccount(qq, pwd);
                }
            }
            
            else if (op == "login") {
                string qq, pwd;
                iss >> qq >> pwd;
                if (qq.empty() || pwd.empty()) {
                    LOG_ERR("用法: login <qq号> <密码>");
                } else {
                    login(qq, pwd);
                }
            }
            
            else if (op == "registerpush") {
                if (!_isLogin || _playerId == 0) {
                    LOG_ERR("请先登录并选择角色!");
                } else {
                    try {
                        _lobbyPrx->registerPush(_playerId);
                        LOG_OK("推送注册成功!");
                    } catch (const std::exception& e) {
                        LOG_ERR("推送注册失败: " << e.what());
                    }
                }
            }
            
            else if (op == "logout") {
                logout();
                LOG_OK("已登出");
            }
            
            // ==================== 角色模块 ====================
            else if (op == "rolelist" || op == "roles") {
                getRoleList();
            }
            
            else if (op == "createrole" || op == "create") {
                string name;
                int job;
                iss >> name >> job;
                if (name.empty()) {
                    LOG_ERR("用法: createrole <角色名> <职业(1战士/2法师/3猎人)>");
                } else if (job < 1 || job > 3) {
                    LOG_ERR("职业必须是 1(战士), 2(法师), 或 3(猎人)");
                } else {
                    createRole(name, job);
                }
            }
            
            else if (op == "selectrole" || op == "select") {
                long roleId;
                iss >> roleId;
                if (roleId <= 0) {
                    LOG_ERR("用法: selectrole <角色ID>");
                } else {
                    selectRole(roleId);
                }
            }
            
            // ==================== 场景模块 ====================
            else if (op == "enter" || op == "enterscene") {
                int sceneId = 1;
                iss >> sceneId;
                enterScene(sceneId);
            }
            
            else if (op == "move") {
                float x, y, z;
                iss >> x >> y >> z;
                if (iss.fail()) {
                    LOG_ERR("用法: move <x> <y> <z>");
                } else {
                    move(x, y, z);
                }
            }
            
            else if (op == "randommove" || op == "random") {
                if (!_inScene) {
                    LOG_ERR("请先进入场景!");
                } else {
                    LOG_INFO("启动随机移动 (后台线程, 3秒间隔)");
                    thread([this]() {
                        while (_running && _inScene) {
                            this_thread::sleep_for(chrono::seconds(3));
                            if (!_running || !_inScene) break;
                            
                            float x = static_cast<float>(rand() % 200 - 100);
                            float y = static_cast<float>(rand() % 200 - 100);
                            float z = static_cast<float>(rand() % 50 - 25);
                            move(x, y, z);
                        }
                    }).detach();
                }
            }
            
            else if (op == "leavescene" || op == "leave") {
                leaveScene();
            }
            
            // ==================== 心跳模块 ====================
            else if (op == "heartbeat" || op == "hb") {
                sendHeartbeat();
            }
            
            else if (op == "starthb" || op == "startheartbeat") {
                int interval = 10;
                iss >> interval;
                startHeartbeat(interval);
            }
            
            else if (op == "stophb" || op == "stopheartbeat") {
                stopHeartbeat();
            }
            
            // ==================== 未知命令 ====================
            else {
                LOG_ERR("未知命令: " << op);
                cout << "输入 'help' 查看可用命令" << endl;
            }
        }
    }
    
private:
    tars::CommunicatorPtr _comm;
    LobbyServantPrx _lobbyPrx;
    LobbyPushCallback* _pushCallback;
    
    long _accountId;
    long _playerId;
    int _sceneId;
    bool _isLogin;
    bool _inScene;
    atomic<bool> _running;
    thread* _heartbeatThread;
};

int main(int argc, char* argv[])
{
    cout << COLOR_BOLD;
    cout << "\n============================================================" << endl;
    cout << "                  TestGameClientV0.2                       " << endl;
    cout << "                  V0.2 模块测试客户端                       " << endl;
    cout << "============================================================" << endl;
    cout << COLOR_RESET << endl;
    
    try {
        TestGameClientV0_2 client;
        client.run();
    } catch (const std::exception& e) {
        cerr << "Fatal exception: " << e.what() << endl;
        return -1;
    }
    
    cout << "客户端已退出." << endl;
    return 0;
}
