find_package(Threads REQUIRED)

add_executable(imap_server
    ImapMain.cpp
    ImapServer.cpp
    ImapSession.cpp
    ImapParser.cpp
)

target_link_libraries(imap_server PRIVATE smtp_proto Threads::Threads)
