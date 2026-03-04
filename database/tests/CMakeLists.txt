find_package(GTest REQUIRED)
include(GoogleTest)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../scheme/001_init_scheme.sql
    ${CMAKE_BINARY_DIR}/scheme/001_init_scheme.sql
    COPYONLY
)

set(TEST_FILE "" CACHE STRING "Single test file (e.g. TestMessageDAL.cpp)")

if(TEST_FILE)
    set(TEST_SOURCES ${TEST_FILE})
else()
    set(TEST_SOURCES)
    foreach(f TestMessageDAL.cpp TestUserDAL.cpp TestAttachmentDAL.cpp
              TestFolderDAL.cpp TestRecipientDAL.cpp
              TestMessageRepository.cpp TestUserRepository.cpp)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${f}")
            list(APPEND TEST_SOURCES ${f})
        endif()
    endforeach()
endif()

add_executable(smtp_tests ${TEST_SOURCES})
target_link_libraries(smtp_tests PRIVATE db_lib GTest::gtest_main)
gtest_discover_tests(smtp_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)