add_library(common_utils_lib
    utils/StringUtils.cpp
    utils/TimeUtils.cpp)

target_include_directories(common_utils_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

if(BUILD_TESTING)
    add_subdirectory(utests)
endif()