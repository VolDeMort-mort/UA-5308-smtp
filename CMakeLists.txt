cmake_minimum_required(VERSION 3.20)
project(smtp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED)

option(BUILD_SERVER "Build the SMTP server" ON)
option(BUILD_MUA "Build the Mail User Agent" ON)
option(BUILD_TESTS "Build tests" OFF)

if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    include(GoogleTest)
    enable_testing()
endif()

add_subdirectory(proto)

if(BUILD_SERVER)
    add_subdirectory(server)
endif()

if(BUILD_MUA)
    add_subdirectory(mua)
endif()

add_subdirectory(logger)
