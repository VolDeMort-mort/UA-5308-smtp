find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(logger_tests test_logger.cpp)

target_link_libraries(logger_tests PRIVATE 
    logger_lib 
    GTest::gtest_main
)
gtest_discover_tests(logger_tests
    DISCOVERY_MODE PRE_TEST
)