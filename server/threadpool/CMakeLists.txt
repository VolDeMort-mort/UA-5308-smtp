cmake_minimum_required(VERSION 3.20)
project(threadpool_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(threadpool_test
    ThreadPoolTest.cpp
    ThreadPool.cpp
    ../../logger/Logger.cpp
    ../../logger/ConsoleStrategy.cpp
    ../../logger/FileStrategy.cpp
)
target_include_directories(threadpool_test PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../../logger
)
target_link_libraries(threadpool_test GTest::gtest_main Threads::Threads)
add_test(NAME ThreadPoolTests COMMAND threadpool_test)
