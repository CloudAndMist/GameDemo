cmake_minimum_required(VERSION 3.5)
project(TestGameClient)

set(CMAKE_CXX_STANDARD 11)

include_directories(/usr/local/tars/cpp/include)
include_directories(${CMAKE_SOURCE_DIR}/src)
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
