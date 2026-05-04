#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>
#include <sstream>
#include "GD.h"
#include "servant/Communicator.h"

using namespace std;
using namespace GD;

class GameClient {
public:
    GameClient(const string& app, const string& server, const string& obj)
        : _app(app), _server(server), _obj(obj), _running(false)
    {
        _comm = new tars::Communicator();
        // Set Tars framework locator for service discovery
        _comm->setProperty("locator", "tars.tarsregistry.QueryObj@tcp -h tars-framework -p 17890");
        _prx = _comm->stringToProxy<GameServantPrx>(_app + "." + _server + "." + _obj);
    }
    
    ~GameClient() {
        if (_running) logout();
    }
    
    bool login(const string& openId) {
        try {
            tars::Int32 ret = _prx->login(openId, _sessionId, _playerId);
            if (ret == 0) {
                _openId = openId;
                _running = true;
                cout << "[Login] Success! sessionId=" << _sessionId << ", playerId=" << _playerId << endl;
                return true;
            }
            cerr << "[Login] Failed, ret=" << ret << endl;
            return false;
        } catch (const std::exception& e) {
            cerr << "[Login] Exception: " << e.what() << endl;
            return false;
        }
    }
    
    bool enterScene() {
        try {
            PlayerInfo self;
            vector<PlayerInfo> players;
            tars::Int32 ret = _prx->enterScene(_sessionId, _playerId, self, players);
            if (ret == 0) {
                _x = self.x;
                _y = self.y;
                cout << "[EnterScene] Success! playerId=" << _playerId << endl;
                cout << "[EnterScene] Current pos: (" << _x << ", " << _y << ")" << endl;
                cout << "[EnterScene] Other players in scene: " << players.size() << endl;
                for (const auto& p : players) {
                    cout << "  - playerId=" << p.playerId << ", openId=" << p.openId 
                         << ", pos=(" << p.x << ", " << p.y << ")" << endl;
                }
                return true;
            }
            cerr << "[EnterScene] Failed, ret=" << ret << endl;
            return false;
        } catch (const std::exception& e) {
            cerr << "[EnterScene] Exception: " << e.what() << endl;
            return false;
        }
    }
    
    bool move(float x, float y) {
        try {
            tars::Int32 ret = _prx->move(_sessionId, x, y);
            if (ret == 0) {
                _x = x; _y = y;
                cout << "[Move] Success! New pos: (" << _x << ", " << _y << ")" << endl;
                return true;
            }
            cerr << "[Move] Failed, ret=" << ret << endl;
            return false;
        } catch (const std::exception& e) {
            cerr << "[Move] Exception: " << e.what() << endl;
            return false;
        }
    }
    
    bool logout() {
        if (!_running) return true;
        try {
            tars::Int32 ret = _prx->logout(_sessionId);
            _running = false;
            if (ret == 0) {
                cout << "[Logout] Success!" << endl;
                return true;
            }
            cerr << "[Logout] Failed, ret=" << ret << endl;
            return false;
        } catch (const std::exception& e) {
            cerr << "[Logout] Exception: " << e.what() << endl;
            return false;
        }
    }
    
    void randomMove() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(-50.0f, 50.0f);
        
        while (_running) {
            this_thread::sleep_for(chrono::seconds(2));
            if (!_running) break;
            
            float x = dist(gen);
            float y = dist(gen);
            move(x, y);
        }
    }
    
    void printHelp() {
        cout << "\n=== Commands ===" << endl;
        cout << "login <openId>   - Login with openId (e.g., login bot_001)" << endl;
        cout << "enter            - Enter game scene" << endl;
        cout << "move <x> <y>     - Move to position" << endl;
        cout << "random           - Start random movement (background)" << endl;
        cout << "stop             - Stop random movement" << endl;
        cout << "logout           - Logout" << endl;
        cout << "help             - Show this help" << endl;
        cout << "quit             - Quit program" << endl;
        cout << "=================" << endl;
    }
    
    void run() {
        printHelp();
        string cmd, openId;
        while (cout << "> ", getline(cin, cmd)) {
            if (cmd.empty()) continue;
            
            // Parse command
            if (cmd == "quit" || cmd == "exit") {
                logout();
                break;
            }
            else if (cmd == "help") {
                printHelp();
            }
            else if (cmd.substr(0, 5) == "login") {
                string oid = cmd.substr(6);
                if (oid.empty()) {
                    cerr << "Usage: login <openId>" << endl;
                } else {
                    logout();
                    login(oid);
                }
            }
            else if (cmd == "enter" || cmd == "enterScene") {
                enterScene();
            }
            else if (cmd.substr(0, 4) == "move") {
                float x, y;
                if (sscanf(cmd.c_str() + 5, "%f %f", &x, &y) == 2) {
                    move(x, y);
                } else {
                    cerr << "Usage: move <x> <y>" << endl;
                }
            }
            else if (cmd == "random") {
                if (!_running) {
                    cerr << "Please login and enter scene first!" << endl;
                } else {
                    cout << "Starting random movement..." << endl;
                    _moveThread = thread(&GameClient::randomMove, this);
                }
            }
            else if (cmd == "stop") {
                cout << "Stopping random movement..." << endl;
                _running = false;
                if (_moveThread.joinable()) _moveThread.join();
                _running = true;
            }
            else if (cmd == "logout") {
                if (_moveThread.joinable()) {
                    _running = false;
                    _moveThread.join();
                }
                logout();
            }
            else {
                cerr << "Unknown command: " << cmd << endl;
                printHelp();
            }
        }
    }
    
private:
    string _app;
    string _server;
    string _obj;
    tars::CommunicatorPtr _comm;
    GameServantPrx _prx;
    string _openId;
    tars::Int64 _sessionId{0};
    tars::Int64 _playerId{0};
    float _x{0}, _y{0};
    atomic<bool> _running;
    thread _moveThread;
};

int main(int argc, char* argv[])
{
    cout << "=== GameDemo Test Client V0.1 ===" << endl;
    cout << "Usage: ./TestGameClient [app] [server] [obj]" << endl;
    
    string app = "GameDemo";
    string server = "GameDemoScene";
    string obj = "GDObj";
    
    if (argc >= 2) app = argv[1];
    if (argc >= 3) server = argv[2];
    if (argc >= 4) obj = argv[3];
    
    cout << "Connecting to: " << app << "." << server << "." << obj << endl;
    
    try {
        GameClient client(app, server, obj);
        client.run();
    } catch (const std::exception& e) {
        cerr << "Fatal exception: " << e.what() << endl;
        return -1;
    }
    
    cout << "Client exited." << endl;
    return 0;
}
