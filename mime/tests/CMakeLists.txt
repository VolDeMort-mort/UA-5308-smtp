add_executable(
    test_mime
    MimeBuilderTest.cpp
    MimeEncoderTest.cpp
    MimeDecoderTest.cpp
    MimeParserTest.cpp
    MimeComposerTest.cpp
)

target_link_libraries(test_mime PRIVATE mime_lib GTest::gtest_main)

target_include_directories(test_mime PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

gtest_discover_tests(test_mime)

if(SMTP_COVERAGE)
    target_code_coverage(test_mime)
endif()
