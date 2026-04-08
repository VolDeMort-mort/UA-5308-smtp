add_executable(
    test_imap
    TestMain.cpp
    ImapCommandTest.cpp
    ImapParserTest.cpp
    ImapUtilsTest.cpp
    ImapResponseTest.cpp
    ImapSessionTest.cpp
    ImapCommandHandlersTest.cpp
    ImapEncryptionHandshakeTest.cpp
    ../src/ImapParser.cpp
    ../src/ImapUtils.cpp
    ../src/ImapCommandDispatcher.cpp
    ../src/ImapServer.cpp
    ../src/ImapSession.cpp
)

target_link_libraries(test_imap PRIVATE imap_lib db_lib GTest::gtest_main GTest::gmock)

target_include_directories(test_imap PRIVATE
    ${CMAKE_SOURCE_DIR}/database/DAL
    ${CMAKE_SOURCE_DIR}/database
)

gtest_discover_tests(test_imap)
