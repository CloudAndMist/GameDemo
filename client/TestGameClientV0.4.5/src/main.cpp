/**
 * TestGameClientV0.4.5 - V0.4.5 移除数据库位置数据 + 一账户一角色
 * 
 * V0.4.5 修改：
 * 1. accountId = playerId (一账户一角色设计)
 * 2. 位置信息不再从数据库存储，由 SceneServer 运行时管理
 * 3. LoginRsp 返回 hasCharacter 标志和 playerInfo
 * 4. 所有需要验证的请求都需要携带 sessionKey
 * 
 * 编译: 在 tars-cpp-compiler 容器中
 *   cd /workspace/client/TestGameClientV0.4.5
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
#include <cmath>

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
        cout << "玩家进入: " << notify.player.playerName;
        cout << " (playerId=" << notify.player.playerId << ")";
        cout << " 位置: (posX=" << notify.player.posX << ", posY=" << notify.player.posY << ", posZ=" << notify.player.posZ << ")" << endl;
    }

    // 玩家移动推送
    virtual void callback_onPlayerMove(tars::Int32 ret, const GameDemo::PlayerMoveNotify& notify)
    {
        cout << COLOR_CYAN << "[PUSH] " << COLOR_RESET;
        cout << "玩家移动: playerId=" << notify.playerId;
        cout << " 位置: (posX=" << notify.x << ", posY=" << notify.y << ", posZ=" << notify.z << ")" << endl;
    }

    // 玩家离开场景推送
    virtual void callback_onPlayerLeave(tars::Int32 ret, const GameDemo::PlayerLeaveNotify& notify)
    {
        cout << COLOR_YELLOW << "[PUSH] " << COLOR_RESET;
        cout << "玩家离开: playerId=" << notify.playerId << endl;
    }

    // V0.4: 玩家掉线推送
    virtual void callback_onPlayerOffline(tars::Int32 ret, const GameDemo::PlayerOfflineNotify& notify)
    {
        cout << COLOR_YELLOW << "[PUSH] " << COLOR_RESET;
        cout << "玩家掉线: playerId=" << notify.playerId << endl;
    }

    // V0.4: 玩家重连推送 (与 onPlayerOffline 区分)
    virtual void callback_onPlayerOnline(tars::Int32 ret, const GameDemo::PlayerEnterNotify& notify)
    {
        cout << COLOR_GREEN << "[PUSH] " << COLOR_RESET;
        cout << "玩家重连上线: " << notify.player.playerName;
        cout << " (playerId=" << notify.player.playerId << ")" << endl;
    }
};

// 错误码说明
string getErrorMsg(int ret) {
    switch (ret) {
        case 0:      return "成功";
        case 1001:   return "账号已存在";
        case 1002:   return "账号不存在";
        case 1003:   return "密码错误";
        case 5001:   return "数据库错误";
        case 9001:   return "无效会话";
        case 9002:   return "角色已存在";  // V0.4.5: 一账户一角色
        case 9003:   return "角色不存在";
        case 9999:   return "服务器繁忙";
        default:     return "未知错误";
    }
}

class TestGameClientV0_4 {
public:
    TestGameClientV0_4()
        : _comm(new tars::Communicator())
        , _pushCallback(nullptr)
        , _accountId(0)
        , _playerId(0)
        , _sessionKey(0)  // V0.4: 会话密钥
        , _sceneId(0)
        , _isLogin(false)
        , _inScene(false)
        , _running(true)
        , _heartbeatThread(nullptr)
        , _autoHeartbeat(false)  // V0.4: 标记是否为自动心跳
        , _curX(0.0f)
        , _curY(0.0f)
    {
        // 设置 Tars 框架定位器
        _comm->setProperty("locator", "tars.tarsregistry.QueryObj@tcp -h tars-framework -p 17890");
        
        // 创建 LobbyServer 代理
        _lobbyPrx = _comm->stringToProxy<LobbyServantPrx>("GameDemo.LobbyServer.LobbyObj");
        
        // 初始化随机数种子
        srand(static_cast<unsigned>(time(nullptr)));

        // 设置 Push 回调 (框架层)
        setPushCallback();
    }

    // 设置 Push 回调 (框架层：定义收到推送后的处理逻辑)
    void setPushCallback() {
        try {
            _pushCallback = new LobbyPushCallback();
            _lobbyPrx->tars_set_push_callback(_pushCallback);
            LOG_INFO("Push 回调设置成功");
        } catch (const std::exception& e) {
            LOG_WARN("Push 回调设置失败: " << string(e.what()));
        }
    }
    
    ~TestGameClientV0_4() {
        cleanup();
    }
    
    void cleanup() {
        _running = false;
        if (_heartbeatThread && _heartbeatThread->joinable()) {
            _heartbeatThread->join();
        }
    }
    
    // ==================== 账号模块 ====================
    
    bool registerAccount(const string& username, const string& password) {
        cout << "\n--- [注册账号] ---" << endl;
        cout << "username: " << username << endl;
        cout << "password: " << password << endl;
        
        try {
            RegisterReq req;
            req.username = username;
            req.password = password;
            RegisterRsp rsp;
            
            tars::Int32 ret = _lobbyPrx->registerAccount(req, rsp);
            
            if (ret == 0) {
                LOG_OK("注册成功! playerId=" << rsp.accountId << " (accountId=playerId)");
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
    
    bool login(const string& username, const string& password) {
        cout << "\n--- [登录] ---" << endl;
        cout << "username: " << username << endl;
        cout << "password: " << string(password.length(), '*') << endl;
        
        try {
            LoginReq req;
            req.username = username;
            req.password = password;
            LoginRsp rsp;
            
            tars::Int32 ret = _lobbyPrx->login(req, rsp);
            
            if (ret == 0) {
                _playerId = rsp.playerId;  // V0.4.5: playerId = accountId
                _accountId = rsp.playerId;
                _sessionKey = rsp.sessionKey;
                _isLogin = true;
                
                LOG_OK("登录成功!");
                cout << "  playerId: " << _playerId << " (accountId=playerId)" << endl;
                cout << "  sessionKey: " << _sessionKey << endl;
                cout << "  hasCharacter: " << (rsp.hasCharacter ? "是" : "否") << endl;
                
                // V0.4.5: 如果已有角色，显示角色信息
                if (rsp.hasCharacter && rsp.playerInfo.playerId > 0) {
                    cout << "\n  [已有角色信息]" << endl;
                    cout << "  角色名称: " << rsp.playerInfo.playerName << endl;
                    cout << "  职业: " << getJobName(rsp.playerInfo.job) << endl;
                    cout << "  等级: " << rsp.playerInfo.level << endl;
                } else if (rsp.hasCharacter && rsp.playerInfo.playerId == 0) {
                    cout << "\n  [已有角色信息, 但playerID为0]" << endl;
                } else {
                    cout << "\n  [没有角色信息, 请使用`createchar <name> <job>`创建角色]" << endl;
                }
                
                // 登录后注册服务端推送
                if (_playerId > 0) {
                    try {
                        _lobbyPrx->registerPush(_playerId);
                        LOG_INFO("服务端推送注册成功!");
                    } catch (const std::exception& e) {
                        LOG_WARN("服务端推送注册失败: " << e.what());
                    }
                }
                
                // 登录后自动启动心跳
                startHeartbeat(3);  // 3秒间隔
                _autoHeartbeat = true;
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
    
    // 辅助函数：获取职业名称
    string getJobName(int job) {
        switch (job) {
            case 1: return "战士";
            case 2: return "法师";
            case 3: return "猎人";
            default: return "未知";
        }
    }
    
    void logout() {
        if (!_isLogin) return;
        cout << "\n--- [登出] ---" << endl;
        LOG_INFO("登出账号: playerId=" << _playerId);
        
        // V0.4.5: 如果在场景中，先离开
        if (_inScene) {
            leaveScene();
        }
        
        _isLogin = false;
        _inScene = false;
        _playerId = 0;
        _accountId = 0;
        _sessionKey = 0;
        _sceneId = 0;
        _curX = 0.0f;
        _curY = 0.0f;
        stopHeartbeat();
    }
    
    // ==================== 角色模块 (V0.4.5: 一账户一角色) ====================
    
    bool createCharacter(const string& playerName, int job) {
        if (!_isLogin || _playerId == 0) {
            LOG_ERR("请先登录!");
            return false;
        }
        
        cout << "\n--- [创建角色] ---" << endl;
        cout << "playerId: " << _playerId << " (accountId=playerId)" << endl;
        cout << "sessionKey: " << _sessionKey << endl;
        cout << "playerName: " << playerName << endl;
        
        cout << "job: " << job << " (" << getJobName(job) << ")" << endl;
        
        try {
            CreateCharacterReq req;
            req.playerId = _playerId;      // V0.4.5: = accountId
            req.sessionKey = _sessionKey;  // V0.4.5: 需要会话密钥
            req.playerName = playerName;
            req.job = job;
            CreateCharacterRsp rsp;
            
            tars::Int32 ret = _lobbyPrx->createCharacter(req, rsp);
            
            if (ret == 0) {
                LOG_OK("创建角色成功!");
                cout << "  playerId: " << rsp.playerInfo.playerId << endl;
                cout << "  playerName: " << rsp.playerInfo.playerName << endl;
                cout << "  job: " << getJobName(rsp.playerInfo.job) << endl;
                cout << "  level: " << rsp.playerInfo.level << endl;
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
    
    // ==================== 场景模块 (V0.4.5) ====================
    
    bool enterScene(int sceneId = 1) {
        if (!_isLogin || _playerId == 0) {
            LOG_ERR("请先登录!");
            return false;
        }
        
        cout << "\n--- [进入场景] ---" << endl;
        cout << "playerId: " << _playerId << endl;
        cout << "sessionKey: " << _sessionKey << endl;
        cout << "sceneId: " << sceneId << endl;
        
        try {
            EnterSceneReq req;
            req.playerId = _playerId;
            req.sessionKey = _sessionKey;  // V0.4.5: 需要会话密钥
            req.sceneId = sceneId;
            EnterSceneRsp rsp;
            
            tars::Int32 ret = _lobbyPrx->enterScene(req, rsp);
            
            if (ret == 0) {
                _inScene = true;
                _sceneId = sceneId;
                _curX = rsp.self.posX;
                _curY = rsp.self.posY;
                LOG_OK("进入场景成功!");
                cout << "  场景ID: " << sceneId << endl;
                cout << "  自身位置: (posX=" << rsp.self.posX << ", posY=" << rsp.self.posY << ", posZ=" << rsp.self.posZ << ")" << endl;
                cout << "  场景内其他玩家: " << rsp.players.size() << endl;
                
                if (!rsp.players.empty()) {
                    cout << endl << "  其他玩家列表:" << endl;
                    cout << setw(10) << "playerId" << setw(15) << "名称" << setw(10) << "等级" 
                         << setw(25) << "位置" << endl;
                    cout << string(65, '-') << endl;
                    for (const auto& p : rsp.players) {
                        cout << setw(10) << p.playerId 
                             << setw(15) << p.playerName 
                             << setw(10) << p.level 
                             << setw(25) << "(posX=" << p.posX << ", posY=" << p.posY << ", posZ=" << p.posZ << ")"
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
        cout << "sessionKey: " << _sessionKey << endl;
        cout << "目标位置: (posX=" << x << ", posY=" << y << ", posZ=" << z << ")" << endl;
        
        try {
            MoveReq req;
            req.playerId = _playerId;
            req.sessionKey = _sessionKey;  // V0.4.5: 需要会话密钥
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
        cout << "sessionKey: " << _sessionKey << endl;
        cout << "sceneId: " << _sceneId << endl;
        
        try {
            LeaveSceneReq req;
            req.playerId = _playerId;
            req.sessionKey = _sessionKey;  // V0.4.5: 需要会话密钥
            req.sceneId = _sceneId;
            LeaveSceneRsp rsp;
            
            tars::Int32 ret = _lobbyPrx->leaveScene(req, rsp);
            
            if (ret == 0) {
                _inScene = false;
                _sceneId = 0;
                _curX = 0.0f;
                _curY = 0.0f;
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
    
    void startHeartbeat(int intervalSeconds = 3) {
        if (_heartbeatThread) {
            LOG_WARN("心跳已在运行!");
            return;
        }
        cout << "启动心跳... (间隔 " << intervalSeconds << " 秒)" << endl;
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
            cout << "停止心跳..." << endl;
            LOG_INFO("停止心跳...");
            _running = false;
            if (_heartbeatThread->joinable()) {
                _heartbeatThread->join();
            }
            delete _heartbeatThread;
            _heartbeatThread = nullptr;
            _running = true;
            
            // V0.4: 如果是自动心跳，提示用户
            if (_autoHeartbeat) {
                LOG_WARN("V0.4: 自动心跳已停止，如需重启请使用 'starthb'");
                _autoHeartbeat = false;
            }
        }
    }
    
    bool sendHeartbeat() {
        if (!_isLogin || _playerId == 0) {
            return false;
        }
        
        try {
            HeartBeatReq req;
            req.playerId = _playerId;
            req.sessionKey = _sessionKey;  // V0.4: 带上 sessionKey
            tars::Int32 ret = _lobbyPrx->heartbeat(req);
            
            if (ret == 0) {
                return true;
            } else {
                cout << COLOR_YELLOW << "[HEARTBEAT] " << COLOR_RESET;
                cout << "playerId=" << _playerId << ", sessionKey=" << _sessionKey << " FAILED, ret=" << ret << endl;
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
        cout << "\n" << string(45, '=') << endl;
        cout << "           当前状态 (V0.4.5)" << endl;
        cout << string(45, '=') << endl;
        cout << "  登录状态: " << (_isLogin ? "已登录" : "未登录") << endl;
        if (_isLogin) {
            cout << "  playerId: " << _playerId << " (accountId=playerId)" << endl;
            cout << "  sessionKey: " << _sessionKey << endl;
            cout << "  场景状态: " << (_inScene ? "场景中" : "大厅") << endl;
            if (_inScene) {
                cout << "  场景ID: " << _sceneId << endl;
                cout << "  当前位置: (posX=" << _curX << ", posY=" << _curY << ")" << endl;
            }
            cout << "  心跳状态: " << (_heartbeatThread ? "运行中" : "未启动");
            if (_autoHeartbeat && _heartbeatThread) {
                cout << " (自动)";
            }
            cout << endl;
        }
        cout << string(45, '=') << endl;
    }
    
    // ==================== 交互式菜单 ====================
    
    void printHelp() {
        cout << "\n" << COLOR_BOLD;
        cout << "============================================================" << endl;
        cout << "      TestGameClientV0.4.5 - V0.4.5 一账户一角色设计        " << endl;
        cout << "============================================================" << endl;
        cout << "  [账号模块]                                                  " << endl;
        cout << "    1. register <user> <pwd>  - 注册账号                     " << endl;
        cout << "    2. login <user> <pwd>      - 登录                       " << endl;
        cout << "    3. registerpush           - 注册服务端推送 (登录后调用)   " << endl;
        cout << "    4. logout                - 登出                         " << endl;
        cout << "                                                              " << endl;
        cout << "  [角色模块 - V0.4.5: 一账户一角色]                         " << endl;
        cout << "    4. createchar <name> <job> - 创建角色 (1战士/2法师/3猎人) " << endl;
        cout << "                                                              " << endl;
        cout << "  [场景模块]                                                  " << endl;
        cout << "    5. enter [sceneId]      - 进入场景 (默认sceneId=1)        " << endl;
        cout << "    6. move <x> <y> <z>    - 移动                          " << endl;
        cout << "    7. randommove           - 随机移动 (后台)                " << endl;
        cout << "    8. leavescene           - 离开场景                     " << endl;
        cout << "                                                              " << endl;
        cout << "  [心跳模块]                                                  " << endl;
        cout << "    9. heartbeat            - 发送一次心跳                   " << endl;
        cout << "   10. starthb [sec]       - 启动心跳 (默认3秒)           " << endl;
        cout << "   11. stophb               - 停止心跳                      " << endl;
        cout << "                                                              " << endl;
        cout << "  [系统]                                                      " << endl;
        cout << "   12. status               - 查看当前状态                   " << endl;
        cout << "   13. help                 - 显示帮助                      " << endl;
        cout << "   14. quit                 - 退出程序                      " << endl;
        cout << "============================================================" << endl;
        cout << COLOR_RESET << endl;
        cout << "  V0.4.5 特性:                                               " << endl;
        cout << "    - accountId = playerId (一账户一角色)                    " << endl;
        cout << "    - 登录返回 hasCharacter 和 playerInfo                   " << endl;
        cout << "    - 位置由 SceneServer 运行时管理                          " << endl;
        cout << "    - 所有请求需携带 sessionKey 验证                         " << endl;
        cout << "============================================================" << endl;
    }
    
    // ==================== 主循环 ====================
    
    void run() {
        cout << COLOR_BOLD;
        cout << "\n============================================================" << endl;
        cout << "      TestGameClientV0.4.5 - V0.4.5 一账户一角色设计        " << endl;
        cout << "                                                              " << endl;
        cout << "  V0.4.5 特性:                                               " << endl;
        cout << "    - accountId = playerId (一账户一角色)                    " << endl;
        cout << "    - 位置由 SceneServer 运行时管理                          " << endl;
        cout << "    - 所有请求需携带 sessionKey 验证                         " << endl;
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
                logout();
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
                string user, pwd;
                iss >> user >> pwd;
                if (user.empty() || pwd.empty()) {
                    LOG_ERR("用法: register <用户名> <密码>");
                } else {
                    registerAccount(user, pwd);
                }
            }
            
            else if (op == "login") {
                string user, pwd;
                iss >> user >> pwd;
                if (user.empty() || pwd.empty()) {
                    LOG_ERR("用法: login <用户名> <密码>");
                } else {
                    login(user, pwd);
                }
            }
            
            else if (op == "registerpush") {
                if (!_isLogin || _playerId == 0) {
                    LOG_ERR("请先登录!");
                } else {
                    try {
                        _lobbyPrx->registerPush(_playerId);
                        LOG_OK("服务端推送注册成功!");
                    } catch (const std::exception& e) {
                        LOG_ERR("服务端推送注册失败: " << e.what());
                    }
                }
            }
            
            else if (op == "logout") {
                logout();
                LOG_OK("已登出");
            }
            
            // ==================== 角色模块 (V0.4.5: 一账户一角色) ====================
            else if (op == "createchar" || op == "create" || op == "createrole") {
                string name;
                int job;
                iss >> name >> job;
                if (name.empty()) {
                    LOG_ERR("用法: createchar <角色名> <职业(1战士/2法师/3猎人)>");
                } else if (job < 1 || job > 3) {
                    LOG_ERR("职业必须是 1(战士), 2(法师), 或 3(猎人)");
                } else {
                    createCharacter(name, job);
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
                    LOG_INFO("启动随机移动 (后台线程, 3秒间隔, 步幅<=5, 范围0-999)");
                    thread([this]() {
                        while (_running && _inScene) {
                            this_thread::sleep_for(chrono::seconds(3));
                            if (!_running || !_inScene) break;
                            
                            // 随机偏移量 [-5, 5]
                            float dx = static_cast<float>((rand() % 11) - 5);
                            float dy = static_cast<float>((rand() % 11) - 5);
                            float z = static_cast<float>(rand() % 10);  // z 保持 0-9 范围
                            
                            // 计算新位置，确保在 [0, 999] 范围内
                            float newX = _curX + dx;
                            float newY = _curY + dy;
                            
                            // 边界环绕 (0-999 循环)
                            newX = fmodf(newX + 1000, 1000);
                            newY = fmodf(newY + 1000, 1000);
                            
                            if (move(newX, newY, z)) {
                                _curX = newX;
                                _curY = newY;
                            }
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
                int interval = 3;
                iss >> interval;
                startHeartbeat(interval);
                _autoHeartbeat = false;  // 手动启动，取消自动标记
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
    long _sessionKey;  // V0.4: 会话密钥
    int _sceneId;
    bool _isLogin;
    bool _inScene;
    atomic<bool> _running;
    thread* _heartbeatThread;
    bool _autoHeartbeat;  // V0.4: 标记是否为自动心跳
    float _curX;  // 当前 X 坐标 (0-999)
    float _curY;  // 当前 Y 坐标 (0-999)
};

int main(int argc, char* argv[])
{
    cout << COLOR_BOLD;
    cout << "\n============================================================" << endl;
    cout << "                 TestGameClientV0.4.5                        " << endl;
    cout << "            V0.4.5 一账户一角色设计客户端                 " << endl;
    cout << "============================================================" << endl;
    cout << COLOR_RESET << endl;
    
    try {
        TestGameClientV0_4 client;
        client.run();
    } catch (const std::exception& e) {
        cerr << "Fatal exception: " << e.what() << endl;
        return -1;
    }
    
    cout << "客户端已退出." << endl;
    return 0;
}
