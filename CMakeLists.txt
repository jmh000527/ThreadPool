cmake_minimum_required(VERSION 3.22)
project(ThreadPool)

set(CMAKE_CXX_STANDARD 20)

include_directories(./include)
#aux_source_directory(./src SRC)

file(GLOB_RECURSE SRC ./src/*.cpp)

add_executable(ThreadPool ${SRC})

# 如果您使用的是 UNIX 或类 UNIX 的系统（Linux、macOS），则需要链接 pthread 库
if (UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(ThreadPool PRIVATE Threads::Threads)
endif ()