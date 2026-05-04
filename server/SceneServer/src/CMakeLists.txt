cmake_minimum_required(VERSION 2.8)

project(GameDemo-SceneServer)

# tars2cpp 需要 include 路径来解析 #include 指令
set(TARS_TOOL_FLAG "--include=/home/tarsproto/GameDemo/GameDemoBase")

# 包含公共头文件目录
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(/usr/local/tars/cpp/include)
include_directories(/home/tarsproto/GameDemo/GameDemoBase)

# 生成 SceneServer 服务
gen_server(GameDemo SceneServer)
