add_executable(test_threadpool ThreadPoolTest.cpp)
target_link_libraries(test_threadpool PRIVATE threadpool_lib GTest::gtest_main)
gtest_discover_tests(test_threadpool)

if(SMTP_COVERAGE)
    target_code_coverage(test_threadpool)
endif()
