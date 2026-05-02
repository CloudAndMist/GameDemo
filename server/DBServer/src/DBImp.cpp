#include "DBImp.h"
#include "util/tc_mysql.h"
#include "util/tc_common.h"
#include <iostream>

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

tars::Int32 DBImp::accountExists(const std::string & qqNumber, tars::Bool & exists, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("accountExists: qqNumber=" << qqNumber << endl);
    try
    {
        TC_Mysql mysql;
        mysql.init(_dbHost, _dbUser, _dbPassword, _dbName, "", _dbPort);
        string sql = "SELECT COUNT(*) FROM accounts WHERE qq_number = '" + mysql.escapeString(qqNumber) + "'";
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

tars::Int32 DBImp::createAccount(const std::string & qqNumber, const std::string & password, tars::Int64 & accountId, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("createAccount: qqNumber=" << qqNumber << endl);
    try
    {
        TC_Mysql mysql;
        mysql.init(_dbHost, _dbUser, _dbPassword, _dbName, "", _dbPort);
        string sql = "INSERT INTO accounts (qq_number, password, create_time) VALUES ('" +
                     mysql.escapeString(qqNumber) + "', '" +
                     mysql.escapeString(password) + "', " +
                     TC_Common::tostr(getCurrentTime()) + ")";
        mysql.execute(sql);
        TC_Mysql::MysqlData data = mysql.queryRecord("SELECT LAST_INSERT_ID() as id");
        accountId = TC_Common::strto<tars::Int64>(data[0]["id"]);
        return 0;
    }
    catch (exception &e)
    {
        TLOG_ERROR("createAccount exception: " << e.what() << endl);
        return ERR_DB_ERROR;
    }
}

tars::Int32 DBImp::getAccountByQQ(const std::string & qqNumber, AccountInfo & account, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("getAccountByQQ: qqNumber=" << qqNumber << endl);
    try
    {
        TC_Mysql mysql;
        mysql.init(_dbHost, _dbUser, _dbPassword, _dbName, "", _dbPort);
        string sql = "SELECT * FROM accounts WHERE qq_number = '" + mysql.escapeString(qqNumber) + "'";
        TC_Mysql::MysqlData data = mysql.queryRecord(sql);
        if (data.size() == 0)
        {
            return ERR_ACCOUNT_NOT_EXISTS;
        }
        account.id = TC_Common::strto<tars::Int64>(data[0]["id"]);
        account.qqNumber = data[0]["qq_number"];
        account.password = data[0]["password"];
        account.createTime = TC_Common::strto<tars::Int64>(data[0]["create_time"]);
        return 0;
    }
    catch (exception &e)
    {
        TLOG_ERROR("getAccountByQQ exception: " << e.what() << endl);
        return ERR_DB_ERROR;
    }
}

tars::Int32 DBImp::createRole(tars::Int64 accountId, const std::string & roleName, tars::Int32 job, RoleInfo & role, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("createRole: accountId=" << accountId << ", roleName=" << roleName << ", job=" << job << endl);
    try
    {
        TC_Mysql mysql;
        mysql.init(_dbHost, _dbUser, _dbPassword, _dbName, "", _dbPort);
        tars::Int64 now = getCurrentTime();
        string sql = "INSERT INTO roles (account_id, role_name, job, level, exp, hp, mp, x, y, scene_id, create_time, last_login_time) VALUES (" +
                     TC_Common::tostr(accountId) + ", '" +
                     mysql.escapeString(roleName) + "', " +
                     TC_Common::tostr(job) + ", 1, 0, 100, 50, 100.0, 100.0, 1, " +
                     TC_Common::tostr(now) + ", " +
                     TC_Common::tostr(now) + ")";
        mysql.execute(sql);
        TC_Mysql::MysqlData data = mysql.queryRecord("SELECT LAST_INSERT_ID() as id");
        tars::Int64 roleId = TC_Common::strto<tars::Int64>(data[0]["id"]);
        return getRole(roleId, role, _current_);
    }
    catch (exception &e)
    {
        TLOG_ERROR("createRole exception: " << e.what() << endl);
        return ERR_DB_ERROR;
    }
}

tars::Int32 DBImp::deleteRole(tars::Int64 roleId, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("deleteRole: roleId=" << roleId << endl);
    try
    {
        TC_Mysql mysql;
        mysql.init(_dbHost, _dbUser, _dbPassword, _dbName, "", _dbPort);
        string sql = "DELETE FROM roles WHERE id = " + TC_Common::tostr(roleId);
        mysql.execute(sql);
        return 0;
    }
    catch (exception &e)
    {
        TLOG_ERROR("deleteRole exception: " << e.what() << endl);
        return ERR_DB_ERROR;
    }
}

tars::Int32 DBImp::getRole(tars::Int64 roleId, RoleInfo & role, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("getRole: roleId=" << roleId << endl);
    try
    {
        TC_Mysql mysql;
        mysql.init(_dbHost, _dbUser, _dbPassword, _dbName, "", _dbPort);
        string sql = "SELECT * FROM roles WHERE id = " + TC_Common::tostr(roleId);
        TC_Mysql::MysqlData data = mysql.queryRecord(sql);
        if (data.size() == 0)
        {
            return ERR_ROLE_NOT_EXISTS;
        }
        role.id = TC_Common::strto<tars::Int64>(data[0]["id"]);
        role.accountId = TC_Common::strto<tars::Int64>(data[0]["account_id"]);
        role.roleName = data[0]["role_name"];
        role.job = TC_Common::strto<tars::Int32>(data[0]["job"]);
        role.level = TC_Common::strto<tars::Int32>(data[0]["level"]);
        role.exp = TC_Common::strto<tars::Int64>(data[0]["exp"]);
        role.hp = TC_Common::strto<tars::Int32>(data[0]["hp"]);
        role.mp = TC_Common::strto<tars::Int32>(data[0]["mp"]);
        role.x = TC_Common::strto<float>(data[0]["x"]);
        role.y = TC_Common::strto<float>(data[0]["y"]);
        role.sceneId = TC_Common::strto<tars::Int32>(data[0]["scene_id"]);
        role.createTime = TC_Common::strto<tars::Int64>(data[0]["create_time"]);
        role.lastLoginTime = TC_Common::strto<tars::Int64>(data[0]["last_login_time"]);
        return 0;
    }
    catch (exception &e)
    {
        TLOG_ERROR("getRole exception: " << e.what() << endl);
        return ERR_DB_ERROR;
    }
}

tars::Int32 DBImp::getRoleList(tars::Int64 accountId, vector<RoleInfo> & roles, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("getRoleList: accountId=" << accountId << endl);
    try
    {
        TC_Mysql mysql;
        mysql.init(_dbHost, _dbUser, _dbPassword, _dbName, "", _dbPort);
        string sql = "SELECT * FROM roles WHERE account_id = " + TC_Common::tostr(accountId);
        TC_Mysql::MysqlData data = mysql.queryRecord(sql);
        roles.clear();
        for (size_t i = 0; i < data.size(); i++)
        {
            RoleInfo role;
            role.id = TC_Common::strto<tars::Int64>(data[i]["id"]);
            role.accountId = TC_Common::strto<tars::Int64>(data[i]["account_id"]);
            role.roleName = data[i]["role_name"];
            role.job = TC_Common::strto<tars::Int32>(data[i]["job"]);
            role.level = TC_Common::strto<tars::Int32>(data[i]["level"]);
            role.exp = TC_Common::strto<tars::Int64>(data[i]["exp"]);
            role.hp = TC_Common::strto<tars::Int32>(data[i]["hp"]);
            role.mp = TC_Common::strto<tars::Int32>(data[i]["mp"]);
            role.x = TC_Common::strto<float>(data[i]["x"]);
            role.y = TC_Common::strto<float>(data[i]["y"]);
            role.sceneId = TC_Common::strto<tars::Int32>(data[i]["scene_id"]);
            role.createTime = TC_Common::strto<tars::Int64>(data[i]["create_time"]);
            role.lastLoginTime = TC_Common::strto<tars::Int64>(data[i]["last_login_time"]);
            roles.push_back(role);
        }
        return 0;
    }
    catch (exception &e)
    {
        TLOG_ERROR("getRoleList exception: " << e.what() << endl);
        return ERR_DB_ERROR;
    }
}

tars::Int32 DBImp::updateRole(const RoleInfo & role, tars::TarsCurrentPtr _current_)
{
    TLOG_DEBUG("updateRole: roleId=" << role.id << endl);
    try
    {
        TC_Mysql mysql;
        mysql.init(_dbHost, _dbUser, _dbPassword, _dbName, "", _dbPort);
        string sql = string("UPDATE roles SET role_name = '") + mysql.escapeString(role.roleName) + "', " +
                     "level = " + TC_Common::tostr(role.level) + ", " +
                     "exp = " + TC_Common::tostr(role.exp) + ", " +
                     "hp = " + TC_Common::tostr(role.hp) + ", " +
                     "mp = " + TC_Common::tostr(role.mp) + ", " +
                     "x = " + TC_Common::tostr(role.x) + ", " +
                     "y = " + TC_Common::tostr(role.y) + ", " +
                     "scene_id = " + TC_Common::tostr(role.sceneId) + ", " +
                     "last_login_time = " + TC_Common::tostr(getCurrentTime()) +
                     " WHERE id = " + TC_Common::tostr(role.id);
        mysql.execute(sql);
        return 0;
    }
    catch (exception &e)
    {
        TLOG_ERROR("updateRole exception: " << e.what() << endl);
        return ERR_DB_ERROR;
    }
}
