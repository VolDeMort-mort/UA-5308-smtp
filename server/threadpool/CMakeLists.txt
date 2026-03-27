cmake_minimum_required(VERSION 3.20)
project(threadpool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_library(threadpool_lib STATIC ThreadPool.cpp)

target_include_directories(threadpool_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/logger
)

target_link_libraries(threadpool_lib PUBLIC
    logger_lib
    Threads::Threads
)

enable_testing()
add_subdirectory(tests)
