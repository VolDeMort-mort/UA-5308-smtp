cmake_minimum_required(VERSION 3.24)
project(smtp VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(BUILD_SERVER   "Build the SMTP server"      ON)
option(BUILD_IMAP     "Build the IMAP server"      ON)
option(BUILD_MUA      "Build the Mail User Agent"  ON)
option(BUILD_DATABASE "Build the Database"         ON)
option(SMTP_TESTING   "Build all tests"            OFF)
option(SMTP_COVERAGE  "Enable code coverage"       OFF)

find_package(Boost    REQUIRED)
find_package(Threads  REQUIRED)
find_package(Sodium   REQUIRED)

if(SMTP_TESTING)
    enable_testing()
    find_package(GTest REQUIRED)
    include(GoogleTest)
endif()

if(SMTP_COVERAGE)
    if(NOT SMTP_TESTING)
        message(FATAL_ERROR "SMTP_COVERAGE requires SMTP_TESTING=ON")
    endif()

    message(STATUS "Coverage enabled")
    include(CodeCoverage)

    add_compile_options(--coverage)
    add_link_options(--coverage)
endif()

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

if(SMTP_COVERAGE)
    get_property(COVERAGE_TARGETS GLOBAL PROPERTY COVERAGE_TARGETS)

    add_custom_target(coverage
        DEPENDS ${COVERAGE_TARGETS}
        COMMAND ${LCOV_PATH} --directory . --zerocounters
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        COMMAND ${LCOV_PATH}
            --directory .
            --capture
            --output-file coverage.info
            --ignore-errors mismatch
            --ignore-errors negative
        COMMAND ${LCOV_PATH}
            --remove coverage.info
                '/usr/*'
                '${CMAKE_SOURCE_DIR}/third_party/*'
                '${CMAKE_BINARY_DIR}/*'
                '*/tests/*'
                '*/test_*'
            --output-file coverage.info
            --ignore-errors unused
        COMMAND ${GENHTML_PATH}
            coverage.info
            --output-directory coverage-html
            --demangle-cpp
            --ignore-errors mismatch
        COMMAND ${CMAKE_COMMAND} -E echo
            "Coverage report: ${CMAKE_BINARY_DIR}/coverage-html/index.html"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating code coverage report..."
    )
endif()

set(CPACK_GENERATOR "RPM")
set(CPACK_COMPONENTS_ALL smtp imap)
set(CPACK_RPM_COMPONENT_INSTALL ON)
include(CPack)
