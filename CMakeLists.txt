cmake_minimum_required(VERSION 3.20)
project(smtp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# UTF-8 support on MSVC
if(MSVC)
    add_compile_options(/utf-8)
endif()

#find_package(Boost REQUIRED)

# --- GTest ---
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
enable_testing()

#option(BUILD_SERVER "Build the SMTP server" ON)
#option(BUILD_IMAP "Build the IMAP server" ON)
#option(BUILD_MUA "Build the Mail User Agent" ON)
#option(BUILD_DATABASE "Build the Database" ON)

add_subdirectory(common)
add_subdirectory(base64)
add_subdirectory(logger)
add_subdirectory(mime)
#add_subdirectory(proto)
#
#if(BUILD_SERVER)
#    add_subdirectory(server)
#endif()
#
#if(BUILD_IMAP)
#    add_subdirectory(imap-server)
#endif()
#
#if(BUILD_MUA)
#    add_subdirectory(mua)
#endif()
#
#if(BUILD_DATABASE)
#    add_subdirectory(database)
#endif()
