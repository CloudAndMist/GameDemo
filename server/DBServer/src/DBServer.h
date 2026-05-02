#ifndef _DBServer_H_
#define _DBServer_H_

#include <iostream>
#include "servant/Application.h"

using namespace tars;

/**
 *
 **/
class DBServer : public Application
{
public:
    /**
     *
     **/
    virtual ~DBServer() {};

    /**
     *
     **/
    virtual void initialize();

    /**
     *
     **/
    virtual void destroyApp();
};

extern DBServer g_app;

////////////////////////////////////////////
#endif
