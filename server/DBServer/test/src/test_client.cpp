#include <iostream>
#include "servant/Communicator.h"
#include "DB.h"

using namespace std;
using namespace tars;
using namespace GameDemo;

int main(int argc, char** argv)
{
    CommunicatorPtr comm = new Communicator();
    
    try {
        // 设置连接超时时间
        comm->setProperty("connect-timeout", "3000");
        
        // 获取 DBServer 代理
        DBServantPrx prx = comm->stringToProxy<DBServantPrx>("GameDemo.DBServer.DBObj@tcp -h tars-node -p 10000");
        
        cout << "Testing DBServer RPC interfaces..." << endl;
        
        // Test 1: accountExists
        cout << "\n1. Testing accountExists(qq: 123456789)..." << endl;
        tars::Bool exists = false;
        tars::Int32 ret = prx->accountExists("123456789", exists);
        cout << "   Result: " << (ret == 0 ? "OK" : "Error") << ", exists=" << (exists ? "true" : "false") << endl;
        
        // Test 2: createAccount
        cout << "\n2. Testing createAccount(qq: 123456789, pwd: test123)..." << endl;
        tars::Int64 accountId = 0;
        ret = prx->createAccount("123456789", "test123", accountId);
        cout << "   Result: " << (ret == 0 ? "OK" : "Error") << ", accountId=" << accountId << endl;
        
        // Test 3: accountExists (again)
        cout << "\n3. Testing accountExists(qq: 123456789) again..." << endl;
        ret = prx->accountExists("123456789", exists);
        cout << "   Result: " << (ret == 0 ? "OK" : "Error") << ", exists=" << (exists ? "true" : "false") << endl;
        
        // Test 4: getAccountByQQ
        cout << "\n4. Testing getAccountByQQ(qq: 123456789)..." << endl;
        AccountInfo account;
        ret = prx->getAccountByQQ("123456789", account);
        if (ret == 0) {
            cout << "   Result: OK, id=" << account.id << ", qq=" << account.qqNumber << endl;
        } else {
            cout << "   Result: Error " << ret << endl;
        }
        
        // Test 5: createRole
        cout << "\n5. Testing createRole(accountId: " << accountId << ")..." << endl;
        RoleInfo role;
        ret = prx->createRole(accountId, "TestRole", 1, role);
        if (ret == 0) {
            cout << "   Result: OK, roleId=" << role.id << ", name=" << role.roleName << ", level=" << role.level << endl;
        } else {
            cout << "   Result: Error " << ret << endl;
        }
        
        // Test 6: getRoleList
        cout << "\n6. Testing getRoleList(accountId: " << accountId << ")..." << endl;
        vector<RoleInfo> roles;
        ret = prx->getRoleList(accountId, roles);
        cout << "   Result: " << (ret == 0 ? "OK" : "Error") << ", roles count=" << roles.size() << endl;
        
        // Test 7: deleteRole
        if (role.id > 0) {
            cout << "\n7. Testing deleteRole(roleId: " << role.id << ")..." << endl;
            ret = prx->deleteRole(role.id);
            cout << "   Result: " << (ret == 0 ? "OK" : "Error") << endl;
        }
        
        cout << "\n=== All tests completed! ===" << endl;
    }
    catch (exception& e) {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }
    
    return 0;
}