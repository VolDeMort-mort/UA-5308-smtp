find_package(Threads REQUIRED)

add_subdirectory(threadpool)

add_executable(smtp_server
    server_main.cpp
    SmtpParser.cpp
    SmtpSession.cpp
)

target_link_libraries(smtp_server PRIVATE smtp_proto db_lib threadpool_lib Threads::Threads logger_lib base64_lib)
