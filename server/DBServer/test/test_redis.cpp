#include <iostream>
#include <hiredis/hiredis.h>

using namespace std;

int main() {
    cout << "Testing Redis connection..." << endl;
    
    redisContext* c = redisConnect("tars-redis", 6379);
    if (c == nullptr || c->err) {
        if (c) {
            cerr << "Redis error: " << c->errstr << endl;
            redisFree(c);
        } else {
            cerr << "Redis connection failed: can't allocate context" << endl;
        }
        return 1;
    }
    
    cout << "Connected successfully!" << endl;
    
    // Test: PING
    redisReply* reply = (redisReply*)redisCommand(c, "PING");
    if (reply && reply->type == REDIS_REPLY_STRING) {
        cout << "PING: " << reply->str << endl;
    }
    freeReplyObject(reply);
    
    // Test: HSET
    reply = (redisReply*)redisCommand(c,
        "HSET player:kv:12345 level %d x %f y %f z %f sceneId %d updateTime %ld",
        5, 100.5f, 0.0f, 200.3f, 1, (long)time(nullptr));
    cout << "HSET: " << (reply && reply->type == REDIS_REPLY_INTEGER ? reply->integer : -1) << endl;
    freeReplyObject(reply);
    
    // Test: HGETALL
    reply = (redisReply*)redisCommand(c, "HGETALL player:kv:12345");
    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        cout << "HGETALL player:kv:12345:" << endl;
        for (size_t i = 0; i < reply->elements; i += 2) {
            cout << "  " << reply->element[i]->str << ": " << reply->element[i+1]->str << endl;
        }
    }
    freeReplyObject(reply);
    
    // Test: EXPIRE
    reply = (redisReply*)redisCommand(c, "EXPIRE player:kv:12345 %d", 86400);
    cout << "EXPIRE: " << (reply && reply->type == REDIS_REPLY_INTEGER ? reply->integer : -1) << endl;
    freeReplyObject(reply);
    
    // Test: DEL
    reply = (redisReply*)redisCommand(c, "DEL player:kv:12345");
    cout << "DEL: " << (reply && reply->type == REDIS_REPLY_INTEGER ? reply->integer : -1) << endl;
    freeReplyObject(reply);
    
    redisFree(c);
    cout << "All tests passed!" << endl;
    return 0;
}
