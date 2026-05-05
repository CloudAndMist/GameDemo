//
// HeartbeatThread.cpp - 心跳检测线程实现
// V0.4: 使用 Tars TC_Thread 封装心跳检测逻辑
//

#include "HeartbeatThread.h"
#include "LobbyServer.h"
#include "LobbyImp.h"

void HeartbeatThread::terminate()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _terminate = true;
    _cond.notify_one();
}

void HeartbeatThread::run()
{
    TLOG_INFO("HeartbeatThread started" << endl);

    while (!_terminate)
    {
        // 使用 condition_variable 等待 5 秒
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _cond.wait_for(lock, std::chrono::seconds(5));
        }

        if (_terminate)
        {
            break;
        }

        try
        {
            if (_app)
            {
                _app->checkHeartbeatTimeout();
            }
        }
        catch (const std::exception& e)
        {
            TLOG_ERROR("HeartbeatThread exception: " << e.what() << endl);
        }
    }

    TLOG_INFO("HeartbeatThread terminated" << endl);
}
