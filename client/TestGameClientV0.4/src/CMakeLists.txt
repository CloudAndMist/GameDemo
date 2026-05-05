set(CMAKE_CXX_STANDARD 11)

# Source files
set(SRC_FILES
    ${CMAKE_SOURCE_DIR}/src/main.cpp
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    /usr/local/tars/cpp/include
    /home/tarsproto/GameDemo/LobbyServer
    /home/tarsproto/GameDemo/GameDemoBase
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    tarsservant
    tarsparse
    tarsutil
    pthread
    dl
)
