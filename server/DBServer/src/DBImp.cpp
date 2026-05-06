// ========== DBImp.cpp ==========
// V0.4.5 数据库服务实现
// 一账户一角色：账号和角色数据合并到 accounts 表

#include "DBImp.h"
#include "util/tc_mysql.h"
#include "util/tc_common.h"
#include <iostream>
#include <sstream>

using namespace std;
using namespace GameDemo;

void DBImp::initialize()
{
    TLOG_DEBUG("DBImp::initialize" << endl);

    _dbHost = "tars-mysql";
    _dbUser = "root";
    _dbPassword = "123456";
    _dbName = "gamedemo";
    _dbPort = 3306;

    try
    {
        TC_Config &conf = getApplication()->getConfig();
        _dbHost = conf.get("/db<host>", _dbHost);
        _dbUser = conf.get("/db<user>", _dbUser);
        _dbPassword = conf.get("/db<password>", _dbPassword);
        _dbName = conf.get("/db<database>", _dbName);
        _dbPort = TC_Common::strto<int>(conf.get("/db<port>", TC_Common::tostr(_dbPort)));
        TLOG_DEBUG("Loaded DB config: host=" << _dbHost << ", db=" << _dbName << endl);
    }
    catch (exception &e)
    {
        TLOG_DEBUG("Using default DB config, exception: " << e.what() << endl);
    }

    if (!connectDB())
    {
        TLOG_ERROR("Failed to connect to database!" << endl);
    }
}

void DBImp::destroy()
{
    TLOG_DEBUG("DBImp::destroy" << endl);
}

bool DBImp::connectDB()
{
    try
    {
        TC_Mysql mysql;
        mysql.init(_dbHost, _dbUser, _dbPassword, _dbName, "", _dbPort);
        mysql.connect();
        TLOG_DEBUG("Database connected successfully!" << endl);
        return true;
    }
    catch (exception &e)
    {
        TLOG_ERROR("DB connection failed: " << e.what() << endl);
        return false;
    }
}

tars::Int64 DBImp::getCurrentTime()
{
    auto now = chrono::system_clock::now();
    return chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count();
}

// ========== 账号操作 ==========

tars::Int32 DBImp::accountExists(const std::string & username, tars::Bool & exists, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("accountExists: username=" << username << endl);
    try
    {
        TC_Mysql mysql;
        mysql.init(_dbHost, _dbUser, _dbPassword, _dbName, "", _dbPort);
        string sql = "SELECT COUNT(*) FROM accounts WHERE username = '" + mysql.escapeString(username) + "'";
        TC_Mysql::MysqlData data = mysql.queryRecord(sql);
        exists = (data.size() > 0 && TC_Common::strto<int>(data[0]["COUNT(*)"]) > 0);
        return 0;
    }
    catch (exception &e)
    {
        TLOG_ERROR("accountExists exception: " << e.what() << endl);
        return ERR_DB_ERROR;
    }
}

tars::Int32 DBImp::createAccount(const std::string & username, const std::string & password, 
                                  tars::Int64 & accountId, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("createAccount: username=" << username << endl);
    try
    {
        TC_Mysql mysql;
        mysql.init(_dbHost, _dbUser, _dbPassword, _dbName, "", _dbPort);
        tars::Int64 now = getCurrentTime();
        string sql = "INSERT INTO accounts (username, password, create_time) VALUES ('" +
                     mysql.escapeString(username) + "', '" +
                     mysql.escapeString(password) + "', " +
                     TC_Common::tostr(now) + ")";
        mysql.execute(sql);
        TC_Mysql::MysqlData data = mysql.queryRecord("SELECT LAST_INSERT_ID() as id");
        accountId = TC_Common::strto<tars::Int64>(data[0]["id"]);
        TLOG_DEBUG("createAccount: accountId=" << accountId << endl);
        return 0;
    }
    catch (exception &e)
    {
        TLOG_ERROR("createAccount exception: " << e.what() << endl);
        return ERR_DB_ERROR;
    }
}

tars::Int32 DBImp::getAccountByName(const std::string & username, AccountInfo & account, 
                                     tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("getAccountByName: username=" << username << endl);
    try
    {
        TC_Mysql mysql;
        mysql.init(_dbHost, _dbUser, _dbPassword, _dbName, "", _dbPort);
        string sql = "SELECT * FROM accounts WHERE username = '" + mysql.escapeString(username) + "'";
        TC_Mysql::MysqlData data = mysql.queryRecord(sql);
        if (data.size() == 0)
        {
            return ERR_ACCOUNT_NOT_EXISTS;
        }
        account.id = TC_Common::strto<tars::Int64>(data[0]["id"]);
        account.username = data[0]["username"];
        account.password = data[0]["password"];
        account.playerName = data[0]["player_name"];
        account.job = TC_Common::strto<tars::Int32>(data[0]["job"]);
        account.level = TC_Common::strto<tars::Int32>(data[0]["level"]);
        account.exp = TC_Common::strto<tars::Int64>(data[0]["exp"]);
        account.hp = TC_Common::strto<tars::Int32>(data[0]["hp"]);
        account.maxHp = TC_Common::strto<tars::Int32>(data[0]["max_hp"]);
        account.mp = TC_Common::strto<tars::Int32>(data[0]["mp"]);
        account.maxMp = TC_Common::strto<tars::Int32>(data[0]["max_mp"]);
        account.createTime = TC_Common::strto<tars::Int64>(data[0]["create_time"]);
        account.lastLoginTime = TC_Common::strto<tars::Int64>(data[0]["last_login_time"]);
        return 0;
    }
    catch (exception &e)
    {
        TLOG_ERROR("getAccountByName exception: " << e.what() << endl);
        return ERR_DB_ERROR;
    }
}

tars::Int32 DBImp::getAccountById(tars::Int64 accountId, AccountInfo & account, 
                                   tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("getAccountById: accountId=" << accountId << endl);
    try
    {
        TC_Mysql mysql;
        mysql.init(_dbHost, _dbUser, _dbPassword, _dbName, "", _dbPort);
        string sql = "SELECT * FROM accounts WHERE id = " + TC_Common::tostr(accountId);
        TC_Mysql::MysqlData data = mysql.queryRecord(sql);
        if (data.size() == 0)
        {
            return ERR_ACCOUNT_NOT_EXISTS;
        }
        account.id = TC_Common::strto<tars::Int64>(data[0]["id"]);
        account.username = data[0]["username"];
        account.password = data[0]["password"];
        account.playerName = data[0]["player_name"];
        account.job = TC_Common::strto<tars::Int32>(data[0]["job"]);
        account.level = TC_Common::strto<tars::Int32>(data[0]["level"]);
        account.exp = TC_Common::strto<tars::Int64>(data[0]["exp"]);
        account.hp = TC_Common::strto<tars::Int32>(data[0]["hp"]);
        account.maxHp = TC_Common::strto<tars::Int32>(data[0]["max_hp"]);
        account.mp = TC_Common::strto<tars::Int32>(data[0]["mp"]);
        account.maxMp = TC_Common::strto<tars::Int32>(data[0]["max_mp"]);
        account.createTime = TC_Common::strto<tars::Int64>(data[0]["create_time"]);
        account.lastLoginTime = TC_Common::strto<tars::Int64>(data[0]["last_login_time"]);
        return 0;
    }
    catch (exception &e)
    {
        TLOG_ERROR("getAccountById exception: " << e.what() << endl);
        return ERR_DB_ERROR;
    }
}

tars::Int32 DBImp::updateAccount(const AccountInfo & account, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("updateAccount: accountId=" << account.id << endl);
    try
    {
        TC_Mysql mysql;
        mysql.init(_dbHost, _dbUser, _dbPassword, _dbName, "", _dbPort);
        
        // 位置信息由 SceneServer 管理，不更新到 DB
        ostringstream oss;
        oss << "UPDATE accounts SET "
            << "username = '" << mysql.escapeString(account.username) << "', "
            << "password = '" << mysql.escapeString(account.password) << "', "
            << "player_name = '" << mysql.escapeString(account.playerName) << "', "
            << "job = " << account.job << ", "
            << "level = " << account.level << ", "
            << "exp = " << account.exp << ", "
            << "hp = " << account.hp << ", "
            << "max_hp = " << account.maxHp << ", "
            << "mp = " << account.mp << ", "
            << "max_mp = " << account.maxMp << ", "
            << "last_login_time = " << account.lastLoginTime << " "
            << "WHERE id = " << account.id;
        string sql = oss.str();
        mysql.execute(sql);
        return 0;
    }
    catch (exception &e)
    {
        TLOG_ERROR("updateAccount exception: " << e.what() << endl);
        return ERR_DB_ERROR;
    }
}

// ========== 角色操作（一账户一角色模式）==========

tars::Int32 DBImp::initCharacter(tars::Int64 accountId, const std::string & playerName, 
                                  tars::Int32 job, AccountInfo & character, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("initCharacter: accountId=" << accountId << ", playerName=" << playerName << ", job=" << job << endl);
    try
    {
        TC_Mysql mysql;
        mysql.init(_dbHost, _dbUser, _dbPassword, _dbName, "", _dbPort);
        tars::Int64 now = getCurrentTime();
        
        // 更新账号的角色数据（位置信息由 SceneServer 管理，不存储到 DB）
        ostringstream oss;
        oss << "UPDATE accounts SET "
            << "player_name = '" << mysql.escapeString(playerName) << "', "
            << "job = " << job << ", "
            << "level = 1, "
            << "exp = 0, "
            << "hp = 100, "
            << "max_hp = 100, "
            << "mp = 50, "
            << "max_mp = 50, "
            << "create_time = " << now << ", "
            << "last_login_time = " << now << " "
            << "WHERE id = " << accountId;
        string sql = oss.str();
        mysql.execute(sql);
        
        // 返回更新后的角色信息
        return getCharacter(accountId, character, _current_);
    }
    catch (exception &e)
    {
        TLOG_ERROR("initCharacter exception: " << e.what() << endl);
        return ERR_DB_ERROR;
    }
}

tars::Int32 DBImp::getCharacter(tars::Int64 playerId, AccountInfo & character, 
                                 tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("getCharacter: playerId=" << playerId << endl);
    // V0.4.5: playerId = accountId，直接查询 accounts 表
    return getAccountById(playerId, character, _current_);
}

tars::Int32 DBImp::updateCharacter(const AccountInfo & character, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("updateCharacter: playerId=" << character.id << endl);
    // V0.4.5: 角色数据就是账号数据
    return updateAccount(character, _current_);
}
