/**
 * LoadBotV0.6 - 压测机器人客户端
 * 
 * 功能：
 * 1. 批量 Bot 登录 (预创建账号，统一 scene 1)
 * 2. 初始分散移动 + 随机移动 (异步调用)
 * 3. Push 广播接收与统计
 * 4. 心跳保活
 * 5. 性能指标自动保存 (JSON)
 * 
 * 编译: 在 tars-cpp-compiler 容器中
 *   cd /workspace/client/LoadBotV0.6
 *   mkdir -p build && cd build
 *   cmake .. && make
 * 
 * 运行:
 *   ./LoadBotV0_6 <botCount> [moveIntervalSec] [sceneId]
 *   示例: ./LoadBotV0_6 100 2 1
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
#include <fstream>
#include <algorithm>
#include <map>
#include <mutex>
#include <cstring>
#include <sys/stat.h>

#include "GameDemoBase.h"
#include "Lobby.h"
#include "Push.h"
#include "servant/Communicator.h"

using namespace std;
using namespace GameDemo;

// ==================== 辅助函数 ====================

string makeBotUsername(int index) {
    char buf[16];
    snprintf(buf, sizeof(buf), "bot_%04d", index);
    return string(buf);
}

// ==================== Push 广播类型 ====================

enum class PushType { ENTER, MOVE, LEAVE, OFFLINE, ONLINE };

// ==================== MetricsCollector (单例) ====================

class MetricsCollector {
public:
    static MetricsCollector& instance() {
        static MetricsCollector inst;
        return inst;
    }

    void recordMoveLatency(int64_t latencyMs) {
        lock_guard<mutex> lk(_mtx);
        _moveLatencies.push_back(latencyMs);
        _totalMoves++;
    }

    void recordPushNotify(PushType type) {
        lock_guard<mutex> lk(_mtx);
        _pushCounts[type]++;
    }

    void recordLoginResult(bool success, int64_t latencyMs) {
        lock_guard<mutex> lk(_mtx);
        if (success) {
            _loginSuccess++;
            _loginLatencies.push_back(latencyMs);
        } else {
            _loginFail++;
        }
    }

    void setOnlineBots(int count) {
        _onlineBots = count;
    }

    void setParams(int botCount, int moveFreqSec, int sceneId, int durationSec) {
        _botCount = botCount;
        _moveFreqSec = moveFreqSec;
        _sceneId = sceneId;
        _durationSec = durationSec;
    }

    // 初始化输出文件路径（启动时调用一次）
    void initRunDir() {
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);
        char dateBuf[32];
        strftime(dateBuf, sizeof(dateBuf), "%Y_%m_%d_%H_%M_%S", t);
        mkdir("metrics", 0755);

        snprintf(_filePath, sizeof(_filePath), "metrics/%d_bot_%d_freqSec_%d_scene_%d_dur_%s.json",
                 _botCount, _moveFreqSec, _sceneId, _durationSec, dateBuf);
    }

    // 最终保存（运行结束时调用一次）
    void finalSave() {
        saveToFile();
    }

private:
    MetricsCollector() : _onlineBots(0), _sceneId(1), _botCount(0), _moveFreqSec(1), _durationSec(0) {
    }
    ~MetricsCollector() {}
    MetricsCollector(const MetricsCollector&) = delete;
    MetricsCollector& operator=(const MetricsCollector&) = delete;

    void saveToFile() {
        lock_guard<mutex> lk(_mtx);

        // 时间戳
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);
        char timeBuf[32];
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%dT%H:%M:%S", t);

        ofstream ofs(_filePath);
        if (!ofs.is_open()) {
            cerr << "[METRICS] Failed to open " << _filePath << endl;
            return;
        }

        // 计算移动延迟统计
        double moveAvg = 0, moveP50 = 0, moveP99 = 0, moveMax = 0;
        if (!_moveLatencies.empty()) {
            vector<int64_t> sorted = _moveLatencies;
            sort(sorted.begin(), sorted.end());
            int64_t sum = 0;
            for (auto v : sorted) sum += v;
            moveAvg = (double)sum / sorted.size();
            moveP50 = (double)sorted[sorted.size() / 2];
            size_t p99Idx = (size_t)(sorted.size() * 0.99);
            moveP99 = (double)sorted[min(p99Idx, sorted.size() - 1)];
            moveMax = (double)sorted.back();
        }

        // 计算登录延迟统计
        double loginAvg = 0;
        if (!_loginLatencies.empty()) {
            int64_t sum = 0;
            for (auto v : _loginLatencies) sum += v;
            loginAvg = (double)sum / _loginLatencies.size();
        }

        int64_t pushMove = _pushCounts[PushType::MOVE];
        int64_t pushEnter = _pushCounts[PushType::ENTER];
        int64_t pushLeave = _pushCounts[PushType::LEAVE];
        int64_t pushOffline = _pushCounts[PushType::OFFLINE];

        double pushPerBotPerSec = 0;
        if (_onlineBots > 0 && _durationSec > 0) {
            pushPerBotPerSec = (double)pushMove / _onlineBots / _durationSec;
        }

        ofs << "{\n";
        ofs << "    \"timestamp\": \"" << timeBuf << "\",\n";
        ofs << "    \"onlineBots\": " << _onlineBots << ",\n";
        ofs << "    \"scene\": " << _sceneId << ",\n";
        ofs << "    \"login\": {\n";
        ofs << "        \"success\": " << _loginSuccess << ",\n";
        ofs << "        \"fail\": " << _loginFail << ",\n";
        ofs << "        \"avgLatencyMs\": " << (int)loginAvg << "\n";
        ofs << "    },\n";
        ofs << "    \"move\": {\n";
        ofs << "        \"totalCount\": " << _totalMoves << ",\n";
        ofs << "        \"avgLatencyMs\": " << (int)moveAvg << ",\n";
        ofs << "        \"p50LatencyMs\": " << (int)moveP50 << ",\n";
        ofs << "        \"p99LatencyMs\": " << (int)moveP99 << ",\n";
        ofs << "        \"maxLatencyMs\": " << (int)moveMax << "\n";
        ofs << "    },\n";
        ofs << "    \"push\": {\n";
        ofs << "        \"moveNotify\": " << pushMove << ",\n";
        ofs << "        \"enterNotify\": " << pushEnter << ",\n";
        ofs << "        \"leaveNotify\": " << pushLeave << ",\n";
        ofs << "        \"offlineNotify\": " << pushOffline << ",\n";
        ofs << "        \"avgPerBotPerSec\": " << fixed << setprecision(2) << pushPerBotPerSec << "\n";
        ofs << "    }\n";
        ofs << "}\n";
        ofs.close();

        cout << "[METRICS] Saved to " << _filePath
             << " moveCount=" << _totalMoves
             << " p99=" << (int)moveP99 << "ms" << endl;
    }

    mutex _mtx;
    vector<int64_t> _moveLatencies;
    map<PushType, int64_t> _pushCounts;
    int64_t _loginSuccess = 0;
    int64_t _loginFail = 0;
    vector<int64_t> _loginLatencies;
    int64_t _totalMoves = 0;
    int _onlineBots;
    int _sceneId;
    int _botCount = 0;
    int _moveFreqSec = 1;
    int _durationSec = 0;
    char _filePath[256] = {};
};

// ==================== BotMoveCallback (异步移动回调) ====================

class LoadBot;  // forward declaration

class BotMoveCallback : public LobbyServantPrxCallback {
public:
    BotMoveCallback(chrono::steady_clock::time_point sendTime, LoadBot* bot);

    virtual void callback_move(tars::Int32 ret, const MoveRsp& rsp) override;
    virtual void callback_move_exception(tars::Int32 ret) override;

private:
    chrono::steady_clock::time_point _sendTime;
    LoadBot* _bot;
};

// ==================== BotPushCallback (Push 广播回调) ====================

class BotPushCallback : public GameDemo::Lobby2ClientPushPrxCallback {
public:
    BotPushCallback(LoadBot* bot) : _bot(bot) {}

    virtual void callback_onPlayerEnter(tars::Int32 ret, const PlayerEnterNotify& notify) {
        MetricsCollector::instance().recordPushNotify(PushType::ENTER);
    }

    virtual void callback_onPlayerMove(tars::Int32 ret, const PlayerMoveNotify& notify) {
        MetricsCollector::instance().recordPushNotify(PushType::MOVE);
    }

    virtual void callback_onPlayerLeave(tars::Int32 ret, const PlayerLeaveNotify& notify) {
        MetricsCollector::instance().recordPushNotify(PushType::LEAVE);
    }

    virtual void callback_onPlayerOffline(tars::Int32 ret, const PlayerOfflineNotify& notify) {
        MetricsCollector::instance().recordPushNotify(PushType::OFFLINE);
    }

    virtual void callback_onPlayerOnline(tars::Int32 ret, const PlayerEnterNotify& notify) {
        MetricsCollector::instance().recordPushNotify(PushType::ONLINE);
    }

private:
    LoadBot* _bot;
};

// ==================== Bot 状态机 ====================

enum class BotState {
    CONNECTING,
    LOGGING_IN,
    REGISTERING_PUSH,
    IDLE,
    MOVING,
    LOGGING_OUT,
    FAILED,
};

// ==================== LoadBot 类 ====================

class LoadBot {
public:
    LoadBot(int botIndex, int sceneId = 1)
        : _botIndex(botIndex)
        , _sceneId(sceneId)
        , _state(BotState::CONNECTING)
        , _playerId(0)
        , _sessionKey(0)
        , _scatterDone(false)
        , _curX(0.0f)
        , _curY(0.0f)
        , _heartbeatRunning(false)


        , _running(true)
    {
        _username = makeBotUsername(botIndex);
        _password = "bot123";
    }

    ~LoadBot() {
        _running = false;
    }

    // 完整初始化链路: login → registerPush → setCallback → heartbeat → enterScene → scatterMove
    bool initialize(tars::CommunicatorPtr comm, LobbyServantPrx lobbyPrx) {
        _comm = comm;
        _lobbyPrx = lobbyPrx;

        try {
            // 1. login
            _state = BotState::LOGGING_IN;
            auto loginStart = chrono::steady_clock::now();

            LoginReq req;
            req.username = _username;
            req.password = _password;
            LoginRsp rsp;

            tars::Int32 ret = _lobbyPrx->login(req, rsp);
            auto loginEnd = chrono::steady_clock::now();
            int64_t loginMs = chrono::duration_cast<chrono::milliseconds>(loginEnd - loginStart).count();

            if (ret != 0) {
                MetricsCollector::instance().recordLoginResult(false, loginMs);
                _state = BotState::FAILED;
                cerr << "[BOT " << _botIndex << "] login failed: ret=" << ret << endl;
                return false;
            }

            _playerId = rsp.playerId;
            _sessionKey = rsp.sessionKey;
            MetricsCollector::instance().recordLoginResult(true, loginMs);

            // 2. registerPush
            _state = BotState::REGISTERING_PUSH;
            _lobbyPrx->registerPush(_playerId);

            // 3. tars_set_push_callback
            _pushCallback = new BotPushCallback(this);
            _lobbyPrx->tars_set_push_callback(_pushCallback);

            // 4. startHeartbeat
            startHeartbeat(3);

            // 5. enterScene
            EnterSceneReq enterReq;
            enterReq.playerId = _playerId;
            enterReq.sessionKey = _sessionKey;
            enterReq.sceneId = _sceneId;
            EnterSceneRsp enterRsp;

            ret = _lobbyPrx->enterScene(enterReq, enterRsp);
            if (ret != 0) {
                cerr << "[BOT " << _botIndex << "] enterScene failed: ret=" << ret << endl;
                _state = BotState::FAILED;
                return false;
            }

            _curX = enterRsp.self.posX;
            _curY = enterRsp.self.posY;

            // 6. scatterMove: 初始分散移动到随机位置
            _state = BotState::MOVING;
            randomMove();  // 首次移动即分散

            return true;

        } catch (const std::exception& e) {
            cerr << "[BOT " << _botIndex << "] initialize exception: " << e.what() << endl;
            _state = BotState::FAILED;
            return false;
        }
    }

    // 随机移动 (异步调用)
    void randomMove() {
        // 并发窗口限制：允许最多 MAX_INFLIGHT 个未完成请求，防止请求洪泛
        if (_moveInflight >= MAX_INFLIGHT) return;

        if (!_scatterDone) {
            // 初始分散: 跳到随机位置 [0~999, 0~999, 0]
            _curX = static_cast<float>(rand() % 1000);
            _curY = static_cast<float>(rand() % 1000);
            _scatterDone = true;
        } else {
            // 正常随机移动: 步长 ±5，保持分散
            _curX = max(0.0f, min(999.0f, _curX + static_cast<float>((rand() % 11) - 5)));
            _curY = max(0.0f, min(999.0f, _curY + static_cast<float>((rand() % 11) - 5)));
        }

        MoveReq req;
        req.playerId = _playerId;
        req.sessionKey = _sessionKey;
        req.x = _curX;
        req.y = _curY;
        req.z = 0;  // 固定 z=0

        _moveInflight++;
        auto sendTime = chrono::steady_clock::now();
        _lobbyPrx->async_move(new BotMoveCallback(sendTime, this), req);
    }

    // 异步回调中记录延迟
    void onMoveResponse(tars::Int32 ret, const MoveRsp& rsp,
                        chrono::steady_clock::time_point sendTime) {
        _moveInflight--;
        auto latencyMs = chrono::duration_cast<chrono::milliseconds>(
            chrono::steady_clock::now() - sendTime).count();
        MetricsCollector::instance().recordMoveLatency(latencyMs);
    }

    void onMoveException() {
        _moveInflight--;
    }

    // 登出 (异步调用服务端 logout 接口，不等待响应)
    void logout() {
        if (_state == BotState::LOGGING_OUT || _state == BotState::FAILED) return;
        _state = BotState::LOGGING_OUT;
        _running = false;
        _heartbeatRunning = false;  // 仅设标志，不 join

        try {
            _lobbyPrx->tars_timeout(1000);  // 缩短超时到 1 秒
            _lobbyPrx->logout(_playerId, _sessionKey);
        } catch (...) {}
    }

    // 等待心跳线程结束（在 delete 前调用）
    void joinHeartbeat() {
        if (_heartbeatThread && _heartbeatThread->joinable()) {
            _heartbeatThread->join();
        }
        delete _heartbeatThread;
        _heartbeatThread = nullptr;
    }

    BotState getState() const { return _state; }
    int getBotIndex() const { return _botIndex; }

private:
    void startHeartbeat(int intervalSeconds) {
        if (_heartbeatRunning) return;
        _heartbeatRunning = true;
        _heartbeatThread = new thread([this, intervalSeconds]() {
            while (_running && _heartbeatRunning) {
                this_thread::sleep_for(chrono::seconds(intervalSeconds));
                if (!_running || !_heartbeatRunning) break;
                try {
                    HeartBeatReq req;
                    req.playerId = _playerId;
                    req.sessionKey = _sessionKey;
                    _lobbyPrx->heartbeat(req);
                } catch (...) {}
            }
        });
    }

    void stopHeartbeat() {
        _heartbeatRunning = false;
        if (_heartbeatThread && _heartbeatThread->joinable()) {
            _heartbeatThread->join();
        }
        delete _heartbeatThread;
        _heartbeatThread = nullptr;
    }

    // 成员
    int _botIndex;
    string _username;
    string _password;
    int _sceneId;

    BotState _state;
    long _playerId;
    long _sessionKey;

    tars::CommunicatorPtr _comm;
    LobbyServantPrx _lobbyPrx;
    BotPushCallback* _pushCallback = nullptr;

    // 心跳
    thread* _heartbeatThread = nullptr;
    bool _heartbeatRunning;

    // 位置
    float _curX;
    float _curY;
    bool _scatterDone;

    // 移动并发控制
    static constexpr int MAX_INFLIGHT = 3;  // 每个bot最多3个并发move请求
    atomic<int> _moveInflight{0};

    // 位置
    atomic<bool> _running;
};

// ==================== BotMoveCallback 实现 ====================

BotMoveCallback::BotMoveCallback(chrono::steady_clock::time_point sendTime, LoadBot* bot)
    : _sendTime(sendTime), _bot(bot) {}

void BotMoveCallback::callback_move(tars::Int32 ret, const MoveRsp& rsp) {
    _bot->onMoveResponse(ret, rsp, _sendTime);
}

void BotMoveCallback::callback_move_exception(tars::Int32 ret) {
    _bot->onMoveException();
}

// ==================== 主程序 ====================

int main(int argc, char* argv[])
{
    cout << "\n============================================================" << endl;
    cout << "                 LoadBotV0.6 - 压测机器人                    " << endl;
    cout << "============================================================" << endl;

    if (argc < 2) {
        cerr << "用法: ./LoadBotV0_6 <botCount> [moveFreqSec] [sceneId] [durationSec]" << endl;
        cerr << "示例: ./LoadBotV0_6 100 2 1 60" << endl;
        cerr << "  moveFreqSec=每秒移动次数(默认1, 范围1~60)" << endl;
        cerr << "  durationSec=0 表示无限运行(默认60秒)" << endl;
        return 1;
    }

    int botCount = atoi(argv[1]);
    int moveFreqSec = (argc >= 3) ? atoi(argv[2]) : 1;
    int sceneId = (argc >= 4) ? atoi(argv[3]) : 1;
    int durationSec = (argc >= 5) ? atoi(argv[4]) : 60;

    if (botCount <= 0 || botCount > 1000) {
        cerr << "botCount 范围: 1~1000" << endl;
        return 1;
    }
    if (moveFreqSec < 1 || moveFreqSec > 60) {
        cerr << "moveFreqSec 范围: 1~60" << endl;
        return 1;
    }

    int moveIntervalMs = 1000 / moveFreqSec;

    cout << "  Bot 数量: " << botCount << endl;
    cout << "  移动频率: " << moveFreqSec << " 次/秒 (间隔" << moveIntervalMs << "ms)" << endl;
    cout << "  场景 ID: " << sceneId << endl;
    cout << "  运行时长: " << (durationSec > 0 ? to_string(durationSec) + " 秒" : "无限") << endl;
    cout << "  账号规则: bot_0001 ~ bot_" << setw(4) << setfill('0') << botCount << endl;
    cout << "  统一密码: bot123" << endl;
    cout << "============================================================\n" << endl;

    srand(static_cast<unsigned>(time(nullptr)));

    // 初始化 Tars 通信器
    tars::CommunicatorPtr comm = new tars::Communicator();
    comm->setProperty("locator", "tars.tarsregistry.QueryObj@tcp -h tars-framework -p 17890");

    LobbyServantPrx lobbyPrx = comm->stringToProxy<LobbyServantPrx>("GameDemo.LobbyServer.LobbyObj");

    // 配置 MetricsCollector
    MetricsCollector::instance().setParams(botCount, moveFreqSec, sceneId, durationSec);
    MetricsCollector::instance().initRunDir();


    // 创建并初始化所有 Bot (分批登录: 每 100ms 创建 10 个)
    vector<LoadBot*> bots;
    bots.reserve(botCount);
    int successCount = 0;
    int failCount = 0;

    cout << "[MAIN] Starting batch login..." << endl;

    auto totalStart = chrono::steady_clock::now();

    for (int i = 1; i <= botCount; i++) {
        LoadBot* bot = new LoadBot(i, sceneId);
        if (bot->initialize(comm, lobbyPrx)) {
            successCount++;
            bots.push_back(bot);
        } else {
            failCount++;
            delete bot;
        }

        // 每 10 个 Bot 间隔 100ms
        if (i % 10 == 0 && i < botCount) {
            this_thread::sleep_for(chrono::milliseconds(100));
        }

        // 进度输出
        if (i % 50 == 0 || i == botCount) {
            cout << "[MAIN] Login progress: " << i << "/" << botCount
                 << " success=" << successCount << " fail=" << failCount << endl;
        }
    }

    auto totalEnd = chrono::steady_clock::now();
    int64_t totalLoginMs = chrono::duration_cast<chrono::milliseconds>(totalEnd - totalStart).count();
    double loginQPS = (totalLoginMs > 0) ? (successCount * 1000.0 / totalLoginMs) : 0;

    cout << "\n[MAIN] Login complete: " << successCount << " success, " << failCount << " fail"
         << " in " << totalLoginMs << "ms (QPS=" << (int)loginQPS << ")" << endl;

    if (bots.empty()) {
        cerr << "[MAIN] No bots online, exiting." << endl;
        MetricsCollector::instance().finalSave();
        return 1;
    }

    MetricsCollector::instance().setOnlineBots(successCount);

    // 进入移动循环
    cout << "[MAIN] Starting move loop (freq=" << moveFreqSec << "/s, interval=" << moveIntervalMs << "ms)..." << endl;
    if (durationSec > 0) {
        cout << "[MAIN] Auto-stop after " << durationSec << "s.\n" << endl;
    } else {
        cout << "[MAIN] Running forever. Press Ctrl+C to stop.\n" << endl;
    }

    auto testStart = chrono::steady_clock::now();
    while (true) {
        this_thread::sleep_for(chrono::milliseconds(moveIntervalMs));

        // 定时退出检查
        if (durationSec > 0) {
            auto elapsed = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - testStart).count();
            if (elapsed >= durationSec) {
                cout << "\n[MAIN] Duration " << durationSec << "s reached, shutting down..." << endl;
                break;
            }
        }

        for (auto* bot : bots) {
            if (bot->getState() == BotState::MOVING) {
                bot->randomMove();
            }
        }
    }

    // 优雅退出: logout + 保存指标
    cout << "[MAIN] Logging out " << bots.size() << " bots..." << endl;
    for (auto* bot : bots) {
        bot->logout();  // 异步 logout，仅设标志+发异步请求
    }
    // 等待异步 logout 请求发出
    this_thread::sleep_for(chrono::milliseconds(500));
    for (auto* bot : bots) {
        bot->joinHeartbeat();  // 统一 join 心跳线程
        delete bot;
    }
    bots.clear();
    MetricsCollector::instance().finalSave();
    cout << "[MAIN] Shutdown complete." << endl;

    return 0;
}
