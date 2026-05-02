cmake_minimum_required(VERSION 2.8)

project(LobbyServer-LobbyServer)

# 添加公共头文件路径
include_directories(${CMAKE_SOURCE_DIR}/../../GameDemoBase)
include_directories(${CMAKE_SOURCE_DIR}/../DBServer/src)

gen_server(LobbyServer LobbyServer)

# target_link_libraries(mysqlclient ssl crypto nghttp2-static)
