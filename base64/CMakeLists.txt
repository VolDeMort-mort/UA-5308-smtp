add_library(
    base64_lib
    src/Base64Encoder.cpp
    src/Base64Decoder.cpp
    src/Base64StreamEncoder.cpp
    src/Base64StreamDecoder.cpp
    src/AttachmentHandler.cpp
)

target_include_directories(
    base64_lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

option(SMTP_TEST_BASE64 "Build base64 tests" ${SMTP_TESTING})
if(SMTP_TEST_BASE64)
    add_subdirectory(tests)
endif()
