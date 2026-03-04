add_executable(server_tests
    test_server.cpp
)

target_link_libraries(server_tests
    PRIVATE
        server_lib
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(server_tests)
