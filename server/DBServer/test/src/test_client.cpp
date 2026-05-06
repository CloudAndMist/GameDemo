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
        
        cout << "=== Testing DBServer V0.4.5 RPC interfaces ===" << endl;
        
        // Test 1: accountExists
        cout << "\n[1] Testing accountExists(username: testuser)..." << endl;
        tars::Bool exists = false;
        tars::Int32 ret = prx->accountExists("testuser", exists);
        cout << "    Result: ret=" << ret << ", exists=" << (exists ? "true" : "false") << endl;
        
        // Test 2: createAccount
        cout << "\n[2] Testing createAccount(username: testuser, password: 123456)..." << endl;
        tars::Int64 accountId = 0;
        ret = prx->createAccount("testuser", "123456", accountId);
        cout << "    Result: ret=" << ret << ", accountId=" << accountId << endl;
        
        // Test 3: getAccountByName
        cout << "\n[3] Testing getAccountByName(username: testuser)..." << endl;
        AccountInfo account;
        ret = prx->getAccountByName("testuser", account);
        if (ret == 0) {
            cout << "    Result: OK" << endl;
            cout << "    - id: " << account.id << endl;
            cout << "    - username: " << account.username << endl;
            cout << "    - playerName: " << account.playerName << endl;
            cout << "    - job: " << account.job << endl;
            cout << "    - level: " << account.level << endl;
        } else {
            cout << "    Result: Error " << ret << endl;
        }
        
        // Test 4: initCharacter (使用 account.id)
        cout << "\n[4] Testing initCharacter(accountId: " << account.id << ", playerName: TestWarrior, job: 0)..." << endl;
        AccountInfo character;
        ret = prx->initCharacter(account.id, "TestWarrior", 0, character);
        if (ret == 0) {
            cout << "    Result: OK" << endl;
            cout << "    - playerName: " << character.playerName << endl;
            cout << "    - job: " << character.job << " (0=战士)" << endl;
            cout << "    - level: " << character.level << endl;
            cout << "    - hp: " << character.hp << "/" << character.maxHp << endl;
        } else {
            cout << "    Result: Error " << ret << endl;
        }
        
        // Test 5: getCharacter (使用 account.id)
        cout << "\n[5] Testing getCharacter(playerId: " << account.id << ")..." << endl;
        ret = prx->getCharacter(account.id, character);
        if (ret == 0) {
            cout << "    Result: OK" << endl;
            cout << "    - playerName: " << character.playerName << endl;
            cout << "    - level: " << character.level << endl;
        } else {
            cout << "    Result: Error " << ret << endl;
        }
        
        // Test 6: updateCharacter
        cout << "\n[6] Testing updateCharacter(level up to 5)..." << endl;
        character.level = 5;
        character.exp = 1000;
        character.hp = 150;
        character.maxHp = 150;
        ret = prx->updateCharacter(character);
        cout << "    Result: ret=" << ret << endl;
        
        // Test 7: getCharacter (verify update, 使用 account.id)
        cout << "\n[7] Testing getCharacter (verify update)..." << endl;
        ret = prx->getCharacter(account.id, character);
        if (ret == 0) {
            cout << "    Result: OK, level=" << character.level << ", exp=" << character.exp << endl;
        } else {
            cout << "    Result: Error " << ret << endl;
        }
        
        cout << "\n=== All tests completed! ===" << endl;
    }
    catch (exception& e) {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }
    
    return 0;
}
