#ifndef _DBImp_H_
#define _DBImp_H_

#include "servant/Application.h"
#include "DB.h"
#include <string>
#include <map>

using namespace std;
using namespace GameDemo;

/**
 * DBImp implements the DBServant interface
 * V0.4.5: 一账户一角色，账号和角色数据合并
 */
class DBImp : public DBServant
{
public:
    DBImp(){};
    ~DBImp(){};

    virtual void initialize() override;
    virtual void destroy() override;

    // ========== 账号操作 ==========
    virtual tars::Int32 accountExists(const std::string & username, tars::Bool & exists, tars::TarsCurrentPtr _current_) override;
    virtual tars::Int32 createAccount(const std::string & username, const std::string & password, tars::Int64 & accountId, tars::TarsCurrentPtr _current_) override;
    virtual tars::Int32 getAccountByName(const std::string & username, AccountInfo & account, tars::TarsCurrentPtr _current_) override;
    virtual tars::Int32 getAccountById(tars::Int64 accountId, AccountInfo & account, tars::TarsCurrentPtr _current_) override;
    virtual tars::Int32 updateAccount(const AccountInfo & account, tars::TarsCurrentPtr _current_) override;

    // ========== 角色操作（一账户一角色模式）==========
    virtual tars::Int32 initCharacter(tars::Int64 accountId, const std::string & playerName, tars::Int32 job, AccountInfo & character, tars::TarsCurrentPtr _current_) override;
    virtual tars::Int32 getCharacter(tars::Int64 playerId, AccountInfo & character, tars::TarsCurrentPtr _current_) override;
    virtual tars::Int32 updateCharacter(const AccountInfo & character, tars::TarsCurrentPtr _current_) override;

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
