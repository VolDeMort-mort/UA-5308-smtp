add_library(config_lib STATIC src/Config.cpp src/JsonParser.cpp)

target_include_directories(
    config_lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

option(SMTP_TEST_CONFIG "Build config tests" ${SMTP_TESTING})
if(SMTP_TEST_CONFIG)
    add_subdirectory(tests)
endif()
