add_executable(test_common
    StringUtilsTest.cpp
    TimeUtilsTest.cpp
)

target_link_libraries(test_common
    PRIVATE
        common_utils_lib
        GTest::gtest_main
)

gtest_discover_tests(test_common)

if(SMTP_COVERAGE)
    target_code_coverage(test_common)
endif()