cmake_minimum_required(VERSION 3.5)
project(TestHelloClient)

set(CMAKE_CXX_STANDARD 11)

# 包含Tars框架头文件
include_directories(/usr/local/tars/cpp/include)

# 关键：添加 tarsproto 共享目录
include_directories(/home/tarsproto/GameDemo/HelloServer)

# 链接库路径
link_directories(/usr/local/tars/cpp/lib)

# 源文件（注意：不再需要 Hello.cpp，因为使用共享的头文件）
set(SRC_FILES
    src/main.cpp
    # 不再需要 src/Hello.cpp
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
    tarsservant  # 对应 libtarsservant.a
    tarsparse    # 对应 libtarsparse.a
    tarsutil     # 对应 libtarsutil.a
    pthread
    dl
    # 通常不需要 rt，除非明确需要实时扩展功能
)