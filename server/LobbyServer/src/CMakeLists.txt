cmake_minimum_required(VERSION 2.8)

project(GameDemo-LobbyServer)

# 添加公共头文件路径
include_directories(${CMAKE_SOURCE_DIR}/../GameDemoBase)
include_directories(${CMAKE_SOURCE_DIR}/../DBServer/src)
include_directories(${CMAKE_SOURCE_DIR}/../SceneServer/src)  # Scene.tars 引用

# 生成 LobbyServer 服务
gen_server(GameDemo LobbyServer)
