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

add_executable(imap_tests
    tests/TestMain.cpp
    tests/ImapCommandTest.cpp
    tests/ImapParserTest.cpp
    tests/ImapUtilsTest.cpp
    tests/ImapResponseTest.cpp
    tests/ImapSessionTest.cpp
    ImapParser.cpp
    ImapUtils.cpp
    ImapSession.cpp
    ../logger/Logger.cpp
    ../logger/FileStrategy.cpp
    ../logger/ConsoleStrategy.cpp
)

target_link_libraries(imap_tests PRIVATE smtp_proto Threads::Threads /usr/lib/x86_64-linux-gnu/libgtest.a /usr/lib/x86_64-linux-gnu/libgtest_main.a)
target_include_directories(imap_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ../logger)

