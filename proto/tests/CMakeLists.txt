add_executable(test_proto encryption_test.cpp proto_unit_test.cpp secure_channel_mock_test.cpp)
target_link_libraries(test_proto PRIVATE smtp_proto GTest::gtest_main GTest::gmock)
gtest_discover_tests(test_proto)

if(SMTP_COVERAGE)
    target_code_coverage(test_proto)
endif()
