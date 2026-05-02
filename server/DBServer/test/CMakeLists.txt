cmake_minimum_required(VERSION 3.5)
project(TestDBClient)

set(CMAKE_CXX_STANDARD 11)

include_directories(/usr/local/tars/cpp/include)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(/usr/local/tars/cpp/thirdparty/include)
include_directories(/home/tarsproto/GameDemo/DBServer)  # 共享目录：make release 发布到此目录
link_directories(/usr/local/tars/cpp/lib)
link_directories(/usr/local/tars/cpp/thirdparty/lib)

set(SRC_FILES
    src/test_client.cpp
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
    tarsservant
    tarsutil
    mysqlclient
    pthread
    dl
    ssl
    crypto
)
