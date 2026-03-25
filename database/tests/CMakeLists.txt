include(GoogleTest)

set(TEST_SOURCES
    Empty.cpp
#    TestUserDAL.cpp
#    TestFolderDAL.cpp
#    TestMessageDAL.cpp
#    TestRecipientDAL.cpp
#    TestUserRepository.cpp
#    TestMessageRepository.cpp
)

add_executable(smtp_tests ${TEST_SOURCES})

target_include_directories(smtp_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(smtp_tests PRIVATE
    db_lib
    GTest::gtest
    GTest::gtest_main
)

target_compile_definitions(smtp_tests PRIVATE
    SCHEMA_PATH="${CMAKE_BINARY_DIR}/scheme/001_init_scheme.sql"
)

gtest_discover_tests(smtp_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DISCOVERY_MODE POST_BUILD
)