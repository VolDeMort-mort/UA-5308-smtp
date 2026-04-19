add_library(
    imap_lib
    STATIC
    src/ImapServer.cpp
    src/ImapSession.cpp
    src/ImapParser.cpp
    src/ImapUtils.cpp
    src/ImapCommandDispatcher.cpp
)

target_include_directories(imap_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(
    imap_lib
    PUBLIC
        smtp_proto
        Threads::Threads
        logger_lib
        db_lib
        threadpool_lib
        mime_lib
        config_lib
)

add_executable(imap_server src/ImapMain.cpp)
target_link_libraries(imap_server PRIVATE imap_lib)

install(TARGETS imap_server COMPONENT imap RUNTIME DESTINATION bin)

option(SMTP_TEST_IMAP "Build imap tests" ${SMTP_TESTING})
if(SMTP_TEST_IMAP)
    add_subdirectory(tests)
endif()
