find_package(Threads REQUIRED)

add_executable(imap_server
    imap-main.cpp
    imap-server.cpp
    imap-session.cpp
    ImapParser.cpp
)

target_link_libraries(imap_server PRIVATE smtp_proto Threads::Threads)
