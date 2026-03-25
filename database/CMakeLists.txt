cmake_minimum_required(VERSION 3.15)
project(database)

find_package(SQLite3 REQUIRED)

find_path(SODIUM_INCLUDE_DIR sodium.h)
find_library(SODIUM_LIBRARY NAMES sodium libsodium)
if(NOT SODIUM_INCLUDE_DIR OR NOT SODIUM_LIBRARY)
    message(FATAL_ERROR "libsodium not found")
endif()

add_library(db_lib STATIC
    DataBaseManager.cpp
    Entity/Recipient.cpp
    DAL/UserDAL.cpp
    DAL/FolderDAL.cpp
    DAL/MessageDAL.cpp
    DAL/RecipientDAL.cpp
    Repository/UserRepository.cpp
    Repository/MessageRepository.cpp
)

target_include_directories(db_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SODIUM_INCLUDE_DIR}
)

target_link_libraries(db_lib PUBLIC
    SQLite::SQLite3
    ${SODIUM_LIBRARY}
    logger_lib
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../scheme/001_init_scheme.sql")
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/../scheme/001_init_scheme.sql
        ${CMAKE_BINARY_DIR}/scheme/001_init_scheme.sql
        COPYONLY
    )
endif()

add_subdirectory(tests)