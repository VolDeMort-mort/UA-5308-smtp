cmake_minimum_required(VERSION 3.24)
project(smtp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# UTF-8 support on MSVC
if(MSVC)
    add_compile_options(/utf-8)
endif()

# --- GTest ---
include(FetchContent)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    GTest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(GTest)

find_package(Boost REQUIRED)

enable_testing()

option(BUILD_SERVER   "Build the SMTP server"     ON)
option(BUILD_IMAP     "Build the IMAP server"     ON)
option(BUILD_MUA      "Build the Mail User Agent"  ON)
option(BUILD_DATABASE "Build the Database"         ON)

add_subdirectory(config)
add_subdirectory(common)
add_subdirectory(base64)
add_subdirectory(logger)
add_subdirectory(mime)

if(BUILD_SERVER OR BUILD_IMAP OR BUILD_MUA)
    add_subdirectory(proto)
endif()

if(BUILD_SERVER)
   add_subdirectory(server)
endif()

if(BUILD_IMAP OR BUILD_DATABASE)
    add_subdirectory(database)
endif()

if(BUILD_IMAP)
   add_subdirectory(imap-server)
endif()

if(BUILD_MUA)
   add_subdirectory(mua)
endif()