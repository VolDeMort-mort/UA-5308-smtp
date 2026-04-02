add_executable(test_proto encryption_test.cpp)
target_link_libraries(test_proto PRIVATE smtp_proto GTest::gtest_main ws2_32)
gtest_discover_tests(test_proto)
