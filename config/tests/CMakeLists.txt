add_executable(test_config
    JsonParserTest.cpp
    ConfigTest.cpp
)

target_link_libraries(test_config PRIVATE config_lib GTest::gtest_main)

gtest_discover_tests(test_config)

if(SMTP_COVERAGE)
    target_code_coverage(test_config)
endif()
