add_executable(
    test_database
    TestUserRepository.cpp
    TestMessageRepository.cpp
    TestConcurrency.cpp
)

target_link_libraries(test_database PRIVATE db_lib GTest::gtest_main)
gtest_discover_tests(test_database)
