find_package(GTest CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS json system)
find_package(OpenSSL REQUIRED)
include(GoogleTest)

add_executable(api_server_tests
    ApiHandlerUnitTests.cpp
)

target_include_directories(api_server_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/database
    ${PROJECT_SOURCE_DIR}/database/Repository
    ${PROJECT_SOURCE_DIR}/database/DAL
    ${PROJECT_SOURCE_DIR}/database/Entity
    ${PROJECT_SOURCE_DIR}/logger/include
    ${PROJECT_SOURCE_DIR}/server/threadpool
)

target_compile_definitions(api_server_tests PRIVATE
    _WIN32_WINNT=0x0601
    SCHEMA_PATH="${PROJECT_SOURCE_DIR}/database/scheme/001_init_scheme.sql"
)

target_link_libraries(api_server_tests PRIVATE
    api_server_lib
    GTest::gtest_main
)

gtest_discover_tests(api_server_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DISCOVERY_MODE PRE_TEST
)

add_executable(api_server_stress_tests
    ApiServerStressTests.cpp
)

target_include_directories(api_server_stress_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/database
    ${PROJECT_SOURCE_DIR}/database/Repository
    ${PROJECT_SOURCE_DIR}/database/DAL
    ${PROJECT_SOURCE_DIR}/database/Entity
    ${PROJECT_SOURCE_DIR}/logger/include
    ${PROJECT_SOURCE_DIR}/server/threadpool
)

target_compile_definitions(api_server_stress_tests PRIVATE
    _WIN32_WINNT=0x0601
    SCHEMA_PATH="${PROJECT_SOURCE_DIR}/database/scheme/001_init_scheme.sql"
)

target_link_libraries(api_server_stress_tests PRIVATE
    api_server_lib
    GTest::gtest_main
)

gtest_discover_tests(api_server_stress_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DISCOVERY_MODE PRE_TEST
)