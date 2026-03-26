find_package(Threads REQUIRED)

add_executable(imap_server
    src/ImapMain.cpp
    src/ImapServer.cpp
    src/ImapSession.cpp
    src/ImapParser.cpp
    src/ImapUtils.cpp
    src/ImapCommandDispatcher.cpp
)

target_include_directories(imap_server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(imap_server PRIVATE smtp_proto Threads::Threads logger_lib db_lib)
target_link_libraries(imap_server PRIVATE config_lib)

add_subdirectory(tests)
