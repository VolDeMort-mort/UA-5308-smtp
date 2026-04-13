add_executable(test_logger test_logger.cpp)

target_link_libraries(test_logger
    PRIVATE
    logger_lib
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

include(GoogleTest)
gtest_discover_tests(test_logger)

if(SMTP_COVERAGE)
    target_code_coverage(test_logger)
endif()
