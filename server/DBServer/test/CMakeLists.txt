cmake_minimum_required(VERSION 3.0)
project(DBServerTest)

set(CMAKE_CXX_STANDARD 11)

set(THIRDPARTY_PATH "/usr/local/tars/cpp/thirdparty")

include_directories(/usr/local/tars/cpp/include)
include_directories(${THIRDPARTY_PATH}/include)
include_directories(${THIRDPARTY_PATH}/include/mysql)
include_directories(/home/tarsproto/GameDemo/GameDemoBase)
include_directories(/home/tarsproto/GameDemo/DBServer)
include_directories(src)

add_executable(test_redis test_redis.cpp)
add_executable(test_tc_mysql test_tc_mysql.cpp)
add_executable(test_kv_client src/test_kv_client.cpp)

# 链接库 - Tars 静态库必须放在 pthread/rt 前面
target_link_libraries(test_kv_client PRIVATE
    hiredis
    /usr/local/tars/cpp/lib/libtarsservant.a
    /usr/local/tars/cpp/lib/libtarsutil.a
    /usr/local/tars/cpp/lib/libtarsparse.a
    pthread
    rt
    dl
)
