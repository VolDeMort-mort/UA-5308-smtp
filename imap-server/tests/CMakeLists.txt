find_package(GTest REQUIRED)
include(GoogleTest)
include(CTest)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../database/scheme/001_init_scheme.sql
    ${CMAKE_BINARY_DIR}/scheme/001_init_scheme.sql
    COPYONLY
)

add_executable(imap_tests
    TestMain.cpp
    ImapCommandTest.cpp
    ImapParserTest.cpp
    ImapUtilsTest.cpp
    ImapResponseTest.cpp
    ImapSessionTest.cpp
    ImapCommandHandlersTest.cpp
    ../src/ImapParser.cpp
    ../src/ImapUtils.cpp
    ../src/ImapCommandDispatcher.cpp
)

target_link_libraries(imap_tests PRIVATE smtp_proto Threads::Threads GTest::gtest GTest::gtest_main GTest::gmock logger_lib db_lib)
target_link_libraries(imap_tests PRIVATE config_lib)
target_include_directories(imap_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

target_compile_definitions(imap_tests PRIVATE
    SCHEMA_PATH="${CMAKE_BINARY_DIR}/scheme/001_init_scheme.sql"
)

gtest_discover_tests(imap_tests WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
