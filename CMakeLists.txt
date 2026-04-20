cmake_minimum_required(VERSION 3.24)
project(smtp VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(BUILD_SERVER "Build the SMTP server" ON)
option(BUILD_IMAP "Build the IMAP server" ON)
option(BUILD_MUA "Build the Mail User Agent" ON)
option(BUILD_DATABASE "Build the Database" ON)
option(BUILD_API "Build the API server" ON)
option(SMTP_TESTING "Build all tests" OFF)
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)
find_package(Sodium REQUIRED)

if(SMTP_TESTING)
    enable_testing()
    find_package(GTest REQUIRED)
    include(GoogleTest)
endif()

add_subdirectory(third_party)
add_subdirectory(config)
add_subdirectory(common)
add_subdirectory(base64)
add_subdirectory(logger)
add_subdirectory(mime)

if(BUILD_SERVER OR BUILD_IMAP OR BUILD_MUA OR BUILD_API)
    add_subdirectory(proto)
endif()

if(BUILD_API)
    add_subdirectory(api-server)
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

set(CPACK_GENERATOR "RPM")
set(CPACK_COMPONENTS_ALL smtp imap)
set(CPACK_RPM_COMPONENT_INSTALL ON)
include(CPack)
