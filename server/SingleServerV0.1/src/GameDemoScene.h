#ifndef _GameDemoScene_H_
#define _GameDemoScene_H_

#include <iostream>
#include "servant/Application.h"

using namespace tars;

/**
 *
 **/
class GameDemoScene : public Application
{
public:
    /**
     *
     **/
    virtual ~GameDemoScene() {};

    /**
     *
     **/
    virtual void initialize();

    /**
     *
     **/
    virtual void destroyApp();
};

extern GameDemoScene g_app;

////////////////////////////////////////////
#endif
