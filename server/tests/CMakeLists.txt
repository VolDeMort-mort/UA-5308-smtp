add_executable(test_smtp_server SmtpParserTest.cpp SmtpSessionTest.cpp)
target_link_libraries(test_smtp_server PRIVATE smtp_server_lib GTest::gtest_main)
gtest_discover_tests(test_smtp_server)

if(SMTP_COVERAGE)
    target_code_coverage(test_smtp_server)
endif()
