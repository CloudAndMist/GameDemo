#include "DBServer.h"
#include "DBImp.h"
#include "KVImp.h"

using namespace std;

DBServer g_app;

/////////////////////////////////////////////////////////////////
void
DBServer::initialize()
{
    // 加载业务自定义配置文件
    addConfig("GameDemo.DBServer.conf");
    
    // DBServant
    addServant<DBImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".DBObj");
    // KVServant (Redis)
    addServant<KVImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".KVObj");
}
/////////////////////////////////////////////////////////////////
void
DBServer::destroyApp()
{
    //destroy application here:
    //...
}
/////////////////////////////////////////////////////////////////
int
main(int argc, char* argv[])
{
    try
    {
        g_app.main(argc, argv);
        g_app.waitForShutdown();
    }
    catch (std::exception& e)
    {
        cerr << "std::exception:" << e.what() << std::endl;
    }
    catch (...)
    {
        cerr << "unknown exception." << std::endl;
    }
    return -1;
}
/////////////////////////////////////////////////////////////////
