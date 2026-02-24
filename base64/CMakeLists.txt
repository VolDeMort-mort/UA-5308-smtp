add_library(base64_lib
    src/Base64Encoder.cpp
    src/Base64Decoder.cpp
    src/Base64StreamEncoder.cpp
    src/Base64StreamDecoder.cpp
    src/AttachmentHandler.cpp
)

target_include_directories(base64_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

include(CTest)
if(BUILD_TESTING AND BUILD_BASE64_TESTS)
    add_subdirectory(utests)
endif()