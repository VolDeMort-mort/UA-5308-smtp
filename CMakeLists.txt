cmake_minimum_required(VERSION 3.20)
project(smtp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)

option(BUILD_SERVER "Build the SMTP server" ON)
option(BUILD_MUA "Build the Mail User Agent" ON)
option(ENABLE_TESTS "Build tests" OFF)

if(ENABLE_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
endif()

add_subdirectory(proto)

if(BUILD_SERVER)
    add_subdirectory(server)
endif()

if(BUILD_MUA)
    add_subdirectory(mua)
endif()
