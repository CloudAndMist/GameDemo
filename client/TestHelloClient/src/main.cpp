#include <iostream>
#include "Hello.h"  // 现在从 /home/tarsproto/GameDemo/HelloServer 目录引用
#include "servant/Communicator.h"

using namespace std;
using namespace GameDemo;

int main(int argc, char* argv[])
{
    tars::CommunicatorPtr comm = new tars::Communicator();
    
    // 使用服务发现（推荐）
    comm->setProperty("locator", "tars.tarsregistry.QueryObj@tcp -h tars-framework -p 17890");
    
    try
    {
        HelloPrx prx = comm->stringToProxy<HelloPrx>("GameDemo.HelloServer.HelloObj");
        
        string outStr;
        int ret = prx->testHello("<Rumi say hello from client>", outStr);
        
        if(ret == 0)
        {
            cout << "Server response: " << outStr << endl;
        }
        else
        {
            cerr << "Call failed, ret: " << ret << endl;
        }
    }
    catch(const std::exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }
    
    return 0;
}