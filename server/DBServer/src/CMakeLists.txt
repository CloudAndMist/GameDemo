cmake_minimum_required(VERSION 2.8)

project(GameDemo-DBServer)

# 包含共享 tars 协议目录
include_directories(${CMAKE_SOURCE_DIR}/../GameDemoBase)
include_directories(${CMAKE_SOURCE_DIR})

gen_server(GameDemo DBServer)

#target_link_libraries(mysqlclient ssl crypto nghttp2-static)
