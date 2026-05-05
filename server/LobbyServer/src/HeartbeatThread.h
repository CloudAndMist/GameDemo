//
// HeartbeatThread.h - 心跳检测线程封装
// V0.4: 使用 Tars TC_Thread 封装心跳检测逻辑
//

#ifndef FRAMEWORK_HEARTBEATTHREAD_H
#define FRAMEWORK_HEARTBEATTHREAD_H

#include <mutex>
#include <chrono>
#include "util/tc_thread.h"
#include "servant/Application.h"

using namespace tars;

// 前向声明
class LobbyServerApp;

/**
 * HeartbeatThread - 心跳检测线程
 * 继承自 TC_Thread，每 5 秒检测一次玩家心跳超时
 */
class HeartbeatThread : public TC_Thread
{
public:
    // 设置所属的 Application
    void setApp(LobbyServerApp* app) { _app = app; }
    
    // 停止线程
    void terminate();

protected:
    // 线程主循环
    virtual void run() override;

private:
    bool _terminate = false;
    std::mutex _mutex;
    std::condition_variable _cond;
    LobbyServerApp* _app = nullptr;
};

#endif //FRAMEWORK_HEARTBEATTHREAD_H
