find_package(GTest CONFIG REQUIRED)

add_executable(threadpool_test ThreadPoolTest.cpp)
target_link_libraries(threadpool_test PRIVATE threadpool_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(threadpool_test)

message(STATUS "Tests configured: threadpool_test")
