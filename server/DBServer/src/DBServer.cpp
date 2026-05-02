#include "DBServer.h"
#include "DBImp.h"

using namespace std;

DBServer g_app;

/////////////////////////////////////////////////////////////////
void
DBServer::initialize()
{
    //initialize application here:
    //...

    addServant<DBImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".DBObj");
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
