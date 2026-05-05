cmake_minimum_required(VERSION 3.5)
project(TestGameClientV0_4)

set(CMAKE_CXX_STANDARD 11)

include_directories(/usr/local/tars/cpp/include)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(/home/tarsproto/GameDemo/LobbyServer)
include_directories(/home/tarsproto/GameDemo/GameDemoBase)
link_directories(/usr/local/tars/cpp/lib)

set(SRC_FILES
    src/main.cpp
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
    tarsservant
    tarsparse
    tarsutil
    pthread
    dl
)
