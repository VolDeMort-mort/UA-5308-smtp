add_subdirectory(threadpool)

add_library(
    smtp_server_lib
    STATIC
    SmtpParser.cpp
    SmtpSession.cpp
)

target_link_libraries(
    smtp_server_lib
    PUBLIC
        smtp_proto
        db_lib
        logger_lib
        mime_lib
        base64_lib
)

add_executable(smtp_server server_main.cpp)

target_link_libraries(
    smtp_server
    PRIVATE
        smtp_server_lib
        threadpool_lib
        Threads::Threads
)

install(TARGETS smtp_server COMPONENT smtp RUNTIME DESTINATION bin)

option(SMTP_TEST_SERVER "Build SMTP server tests" ${SMTP_TESTING})
if(SMTP_TEST_SERVER)
    add_subdirectory(tests)
endif()
