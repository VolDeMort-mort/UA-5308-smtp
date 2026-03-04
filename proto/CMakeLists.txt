find_path(SODIUM_INCLUDE_DIR sodium.h)
find_library(SODIUM_LIBRARY NAMES sodium libsodium)

if(NOT SODIUM_INCLUDE_DIR OR NOT SODIUM_LIBRARY)
    message(FATAL_ERROR "libsodium not found")
endif()

add_library(smtp_proto INTERFACE)
target_include_directories(smtp_proto INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SODIUM_INCLUDE_DIR}
)
target_link_libraries(smtp_proto INTERFACE Boost::headers ${SODIUM_LIBRARY})

if(BUILD_TESTS)
    add_executable(test_secure_connection
        test_SecureConnection.cpp
        ${CMAKE_SOURCE_DIR}/server/SecureConnection.cpp
        ${CMAKE_SOURCE_DIR}/server/SocketConnection.cpp
    )
    target_link_libraries(test_secure_connection PRIVATE smtp_proto GTest::gtest_main)
    gtest_discover_tests(test_secure_connection)
endif()
