cmake_minimum_required(VERSION 3.24)
project(smtp LANGUAGES CXX)

find_package(GTest REQUIRED)
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
