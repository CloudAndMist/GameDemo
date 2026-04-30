#include "GameDemoScene.h"
#include "GDImp.h"

using namespace std;

GameDemoScene g_app;

/////////////////////////////////////////////////////////////////
void
GameDemoScene::initialize()
{
    //initialize application here:
    //...

    addServant<GDImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".GDObj");
}
/////////////////////////////////////////////////////////////////
void
GameDemoScene::destroyApp()
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
