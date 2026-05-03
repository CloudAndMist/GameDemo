cmake_minimum_required(VERSION 2.8)

project(GameDemo-SceneServer)

# 包含 GameDemoBase 公共协议目录 (Push.h 在此生成)
include_directories(${CMAKE_SOURCE_DIR}/../GameDemoBase)

# 生成 SceneServer 服务
# gen_server 会自动处理 Scene.tars 及 GameDemoBase 目录下的依赖
gen_server(GameDemo SceneServer)
