find_package(Threads REQUIRED)

add_executable(smtp_server
    server_main.cpp
    SMTPServer.cpp
    SMTPServerSession.cpp
    SMTPClientSocket.cpp
    SMTPCommandParser.cpp
    SMTPResponseParser.cpp
    SMTPServerSocket.cpp
)

target_link_libraries(smtp_server PRIVATE smtp_proto Threads::Threads)
