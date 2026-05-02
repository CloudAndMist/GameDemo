cmake_minimum_required(VERSION 2.8)

project(GameDemo-SceneServer)

# 包含 GameDemoBase 公共协议目录
include_directories(${CMAKE_SOURCE_DIR}/../GameDemoBase)

gen_server(GameDemo SceneServer)

#target_link_libraries(mysqlclient ssl crypto nghttp2-static)
