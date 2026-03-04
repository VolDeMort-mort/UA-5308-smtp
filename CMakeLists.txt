cmake_minimum_required(VERSION 3.20)
project(smtp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED)

option(BUILD_SERVER "Build the SMTP server" ON)
option(BUILD_MUA "Build the Mail User Agent" ON)

add_subdirectory(proto)
add_subdirectory(base64)

if(BUILD_SERVER)
    add_subdirectory(server)
endif()

if(BUILD_MUA)
    add_subdirectory(mua)
endif()
