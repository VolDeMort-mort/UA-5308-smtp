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

target_link_libraries(test_imap PRIVATE imap_lib GTest::gtest GTest::gmock)

target_include_directories(test_imap PRIVATE
    ${CMAKE_SOURCE_DIR}/database
    ${CMAKE_SOURCE_DIR}/database/DAL
    ${CMAKE_SOURCE_DIR}/database/Entity
)

gtest_discover_tests(test_imap)

if(SMTP_COVERAGE)
    target_code_coverage(test_imap)
endif()