find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(base64_utests
    tests.cpp
)

target_link_libraries(base64_utests
    PRIVATE
        base64_lib
        GTest::gtest_main
)

gtest_discover_tests(base64_utests)