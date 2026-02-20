find_package(Threads REQUIRED)

add_executable(smtp_server
    server_main.cpp
    SmtpParser.cpp
    SmtpSession.cpp
    SocketAcceptor.cpp
    SocketConnection.cpp
    SocketConnector.cpp
)

target_link_libraries(smtp_server PRIVATE smtp_proto Threads::Threads)
