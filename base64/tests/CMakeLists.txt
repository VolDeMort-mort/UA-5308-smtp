add_executable(test_base64 tests.cpp)
target_link_libraries(test_base64 PRIVATE base64_lib GTest::gtest_main)
gtest_discover_tests(test_base64)

if(SMTP_COVERAGE)
    target_code_coverage(test_base64)
endif()
