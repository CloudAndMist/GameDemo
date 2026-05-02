#include <iostream>
#include "util/tc_mysql.h"

using namespace tars;

int main() {
    std::cout << "Testing TC_Mysql connection..." << std::endl;
    
    try {
        TC_Mysql mysql;
        mysql.init("tars-mysql", "root", "123456", "gamedemo", "", 3306);
        mysql.connect();
        
        std::cout << "Connected successfully!" << std::endl;
        
        // Test: select count(*) from accounts
        TC_Mysql::MysqlData data = mysql.queryRecord("SELECT COUNT(*) as cnt FROM accounts");
        std::cout << "Accounts count: " << data[0]["cnt"] << std::endl;
        
        // Test: select count(*) from roles
        data = mysql.queryRecord("SELECT COUNT(*) as cnt FROM roles");
        std::cout << "Roles count: " << data[0]["cnt"] << std::endl;
        
        // Test: insert an account
        std::string sql = "INSERT INTO accounts (qq_number, password, create_time) VALUES ('123456789', 'test123', 1700000000000)";
        mysql.execute(sql);
        std::cout << "Insert account OK!" << std::endl;
        
        // Test: query the account
        data = mysql.queryRecord("SELECT * FROM accounts WHERE qq_number = '123456789'");
        if (data.size() > 0) {
            std::cout << "Query account OK! id=" << data[0]["id"] << ", qq=" << data[0]["qq_number"] << std::endl;
        }
        
        // Test: delete the account
        mysql.execute("DELETE FROM accounts WHERE qq_number = '123456789'");
        std::cout << "Delete account OK!" << std::endl;
        
        std::cout << "All tests passed!" << std::endl;
        return 0;
    }
    catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}