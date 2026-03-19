cmake_minimum_required(VERSION 3.24)
project(smtp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# UTF-8 support on MSVC
if(MSVC)
    add_compile_options(/utf-8)
endif()

# --- GTest ---
# OVERRIDE_FIND_PACKAGE intercepts all find_package(GTest) calls in
# subdirectories and provides GTest via FetchContent automatically.
include(FetchContent)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    GTest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    OVERRIDE_FIND_PACKAGE
)

enable_testing()

option(BUILD_SERVER   "Build the SMTP server"     ON)
option(BUILD_IMAP     "Build the IMAP server"     ON)
option(BUILD_MUA      "Build the Mail User Agent"  ON)
option(BUILD_DATABASE "Build the Database"         ON)

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
