find_package(GTest REQUIRED)
include(GoogleTest)
include(CTest)

add_executable(imap_tests
    TestMain.cpp
    ImapCommandTest.cpp
    ImapParserTest.cpp
    ImapUtilsTest.cpp
    ImapResponseTest.cpp
    ImapSessionTest.cpp
    ../src/ImapParser.cpp
    ../src/ImapUtils.cpp
    ../src/ImapSession.cpp
)

target_link_libraries(imap_tests PRIVATE smtp_proto Threads::Threads GTest::gtest GTest::gtest_main logger_lib)
target_include_directories(imap_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

gtest_discover_tests(imap_tests)
