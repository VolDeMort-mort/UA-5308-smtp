find_package(SQLite3 REQUIRED)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/init_schema.c
    COMMAND
        ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/scheme/001_init_scheme.sql
        ${CMAKE_CURRENT_BINARY_DIR}/init_schema.sql
    COMMAND xxd -i init_schema.sql init_schema.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scheme/001_init_scheme.sql
    COMMENT "Embedding SQL schema"
)

add_library(
    db_lib
    STATIC
    DataBaseManager.cpp
    ConnectionPool.cpp
    Entity/Recipient.cpp
    DAL/UserDAL.cpp
    DAL/FolderDAL.cpp
    DAL/MessageDAL.cpp
    DAL/RecipientDAL.cpp
    Repository/UserRepository.cpp
    Repository/MessageRepository.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/init_schema.c
)

target_include_directories(db_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
    db_lib
    PUBLIC SQLite::SQLite3 Sodium::Sodium logger_lib config_lib
)

option(SMTP_TEST_DATABASE "Build database tests" ${SMTP_TESTING})
if(SMTP_TEST_DATABASE)
    add_subdirectory(tests)
endif()
