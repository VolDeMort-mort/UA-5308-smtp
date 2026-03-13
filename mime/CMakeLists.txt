add_library(mime_lib
    src/MimeBuilder.cpp
    src/MimeEncoder.cpp
    src/MimeParser.cpp
    src/MimeDecoder.cpp
    src/MimeComposer.cpp
)

target_include_directories(mime_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(mime_lib
    PUBLIC
        base64_lib    
        common_utils_lib 
        logger_lib
)

if(BUILD_TESTING)
    add_subdirectory(utests)
endif()

