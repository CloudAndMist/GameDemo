#ifndef _DBImp_H_
#define _DBImp_H_

#include "servant/Application.h"
#include "DB.h"
#include <string>

using namespace std;
using namespace GameDemo;

/**
 * DBImp implements the DBServant interface
 * Handles all database operations for the game
 */
class DBImp : public DBServant
{
public:
    DBImp(){};
    ~DBImp(){};

    virtual void initialize() override;
    virtual void destroy() override;

    // Account operations
    virtual tars::Int32 accountExists(const std::string & qqNumber, tars::Bool & exists, tars::TarsCurrentPtr _current_) override;
    virtual tars::Int32 createAccount(const std::string & qqNumber, const std::string & password, tars::Int64 & accountId, tars::TarsCurrentPtr _current_) override;
    virtual tars::Int32 getAccountByQQ(const std::string & qqNumber, AccountInfo & account, tars::TarsCurrentPtr _current_) override;

    // Role operations
    virtual tars::Int32 createRole(tars::Int64 accountId, const std::string & roleName, tars::Int32 job, RoleInfo & role, tars::TarsCurrentPtr _current_) override;
    virtual tars::Int32 deleteRole(tars::Int64 roleId, tars::TarsCurrentPtr _current_) override;
    virtual tars::Int32 getRole(tars::Int64 roleId, RoleInfo & role, tars::TarsCurrentPtr _current_) override;
    virtual tars::Int32 getRoleList(tars::Int64 accountId, vector<RoleInfo> & roles, tars::TarsCurrentPtr _current_) override;
    virtual tars::Int32 updateRole(const RoleInfo & role, tars::TarsCurrentPtr _current_) override;

private:
    bool connectDB();
    tars::Int64 getCurrentTime();

private:
    string _dbHost;
    string _dbUser;
    string _dbPassword;
    string _dbName;
    int _dbPort;
};

#endif
