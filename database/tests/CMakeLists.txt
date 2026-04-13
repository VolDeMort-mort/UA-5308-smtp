add_executable(
    test_database
    TestUserRepository.cpp
    TestMessageRepository.cpp
    TestConcurrency.cpp
    TestDataBaseManager.cpp
    TestRecipient.cpp
    TestDAL.cpp
)

target_link_libraries(test_database PRIVATE db_lib GTest::gtest_main)
gtest_discover_tests(test_database)

if(SMTP_COVERAGE)
    target_code_coverage(test_database)
endif()
