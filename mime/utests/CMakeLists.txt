add_executable(mime_tests
    MimeBuilderTest.cpp
    MimeEncoderTest.cpp
    MimeDecoderTest.cpp
    MimeParserTest.cpp
    MimeComposerTest.cpp
)

target_link_libraries(mime_tests
    PRIVATE
        mime_lib
        GTest::gtest_main
)

target_include_directories(mime_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

include(GoogleTest)
gtest_discover_tests(mime_tests)
