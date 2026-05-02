#include <iostream>
#include <mysql/mysql.h>

int main() {
    std::cout << "Testing MySQL connection..." << std::endl;
    
    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL) {
        std::cerr << "mysql_init failed" << std::endl;
        return 1;
    }
    
    if (mysql_real_connect(conn, "tars-mysql", "root", "123456", "gamedemo", 3306, NULL, 0) == NULL) {
        std::cerr << "mysql_real_connect failed: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        return 1;
    }
    
    std::cout << "Connected successfully!" << std::endl;
    
    // Test query: select count(*) from accounts
    if (mysql_query(conn, "SELECT COUNT(*) FROM accounts")) {
        std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        return 1;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result) {
        MYSQL_ROW row = mysql_fetch_row(result);
        std::cout << "Accounts count: " << (row ? row[0] : "0") << std::endl;
        mysql_free_result(result);
    }
    
    // Test query: select count(*) from roles
    if (mysql_query(conn, "SELECT COUNT(*) FROM roles")) {
        std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
    } else {
        result = mysql_store_result(conn);
        if (result) {
            MYSQL_ROW row = mysql_fetch_row(result);
            std::cout << "Roles count: " << (row ? row[0] : "0") << std::endl;
            mysql_free_result(result);
        }
    }
    
    mysql_close(conn);
    std::cout << "Test completed successfully!" << std::endl;
    return 0;
}
