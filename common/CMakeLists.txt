add_library(common_utils_lib utils/StringUtils.cpp utils/TimeUtils.cpp)

target_include_directories(
    common_utils_lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

option(SMTP_TEST_COMMON "Build common tests" ${SMTP_TESTING})
if(SMTP_TEST_COMMON)
    add_subdirectory(tests)
endif()
