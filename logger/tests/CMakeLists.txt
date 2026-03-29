add_executable(test_logger test_logger.cpp)
target_link_libraries(test_logger PRIVATE logger_lib GTest::gtest_main)
gtest_discover_tests(test_logger)
