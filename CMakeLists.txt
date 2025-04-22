cmake_minimum_required(VERSION 3.10)
project(webserver C)

set(CMAKE_C_STANDARD 99)

# 设置源码文件
set(SOURCES
    pidser.c
    threadpool.c
)

# 添加可执行文件
add_executable(webserver ${SOURCES})

# 链接 pthread 库
find_package(Threads REQUIRED)
target_link_libraries(webserver Threads::Threads)
