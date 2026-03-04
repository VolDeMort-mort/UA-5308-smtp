add_executable(proto_tests
    test_proto.cpp
)

target_link_libraries(proto_tests
    PRIVATE
        smtp_proto
        GTest::gtest_main
)

add_custom_command(TARGET proto_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:proto_tests>/certs
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/certs
        $<TARGET_FILE_DIR:proto_tests>/certs
)

include(GoogleTest)
gtest_discover_tests(proto_tests)
