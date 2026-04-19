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
)

target_link_libraries(test_imap PRIVATE imap_lib GTest::gtest_main GTest::gmock)
gtest_discover_tests(test_imap)
