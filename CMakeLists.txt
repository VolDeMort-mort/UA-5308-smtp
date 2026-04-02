cmake_minimum_required(VERSION 3.24)
cmake_policy(SET CMP0167 NEW)

project(smtp VERSION 1.0.0 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

include(CTest)

# COVERAGE

option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)

if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(STATUS "✓ Code coverage enabled")

    function(enable_coverage target)
        target_compile_options(${target} PRIVATE -O0 -g -fprofile-arcs -ftest-coverage)
        target_link_options(${target} PRIVATE -fprofile-arcs -ftest-coverage)
    endfunction()
endif()

# SETTINGS

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(BUILD_SERVER "Build the SMTP server" ON)
option(BUILD_IMAP "Build the IMAP server" ON)
option(BUILD_MUA "Build the Mail User Agent" ON)
option(BUILD_DATABASE "Build the Database" ON)

# 🔥 ВАЖНО: включи тесты по умолчанию или через cmake -D
option(SMTP_TESTING "Build all tests" ON)

# DEPENDENCIES

find_package(Boost REQUIRED)
find_package(Threads REQUIRED)
find_package(Sodium REQUIRED)

if(SMTP_TESTING)
    enable_testing()
endif()

# SUBDIRECTORIES

add_subdirectory(third_party)
add_subdirectory(config)
add_subdirectory(common)
add_subdirectory(base64)
add_subdirectory(logger)
add_subdirectory(mime)

if(BUILD_SERVER OR BUILD_IMAP OR BUILD_MUA)
    add_subdirectory(proto)
endif()

if(BUILD_SERVER)
    add_subdirectory(server)
endif()

if(BUILD_IMAP OR BUILD_DATABASE)
    add_subdirectory(database)
endif()

if(BUILD_IMAP)
    add_subdirectory(imap-server)
endif()

if(BUILD_MUA)
    add_subdirectory(mua)
endif()

# 🔥 ВАЖНО: подключаем тесты
if(SMTP_TESTING)
    add_subdirectory(tests)
endif()

# APPLY COVERAGE

if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")

    if(TARGET smtp_server)
        enable_coverage(smtp_server)
    endif()

    if(TARGET imap_server)
        enable_coverage(imap_server)
    endif()

    if(TARGET smtp_mua)
        enable_coverage(smtp_mua)
    endif()

endif()

# PACKAGING


set(CPACK_GENERATOR "RPM")
set(CPACK_COMPONENTS_ALL smtp imap)
set(CPACK_RPM_COMPONENT_INSTALL ON)
include(CPack)


# COVERAGE REPORT


if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")

    find_program(LCOV lcov REQUIRED)
    find_program(GENHTML genhtml REQUIRED)

    add_custom_target(coverage
        COMMAND ${LCOV} --directory . --capture --output-file coverage.info
        COMMAND ${LCOV} --remove coverage.info '/usr/*' '*/third_party/*' --output-file coverage.info
        COMMAND ${GENHTML} coverage.info --output-directory coverage_report
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating coverage report..."
    )

endif()
