find_package(Threads REQUIRED)

add_executable(imap_server
    ImapMain.cpp
    ImapServer.cpp
    ImapSession.cpp
    ImapParser.cpp
    ImapUtils.cpp
    ../logger/Logger.cpp
    ../logger/FileStrategy.cpp
    ../logger/ConsoleStrategy.cpp
)

target_link_libraries(imap_server PRIVATE smtp_proto Threads::Threads)
