add_library(
    logger_lib
    STATIC
    src/Logger.cpp
    src/FileStrategy.cpp
    src/ConsoleStrategy.cpp
)

target_include_directories(
    logger_lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(logger_lib PRIVATE Threads::Threads)

option(SMTP_TEST_LOGGER "Build logger tests" ${SMTP_TESTING})

if(SMTP_TEST_LOGGER)
    add_subdirectory(tests)
endif()