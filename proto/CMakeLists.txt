find_path(SODIUM_INCLUDE_DIR sodium.h)
find_library(SODIUM_LIBRARY NAMES sodium libsodium)

add_library(smtp_proto STATIC
    SocketAcceptor.cpp
    SocketConnection.cpp
    SocketConnector.cpp
    ClientSecureChannel.cpp
    SecureChannel.cpp
    ServerSecureChannel.cpp
)

target_include_directories(smtp_proto PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SODIUM_INCLUDE_DIR}
)

target_link_libraries(smtp_proto PUBLIC
    Boost::headers
    ${SODIUM_LIBRARY}
    logger_lib
)

if(BUILD_ENCRYPTION_TESTS)
    find_package(GTest CONFIG REQUIRED)
    add_executable(encryption_test encryption_test.cpp)
    target_link_libraries(encryption_test PRIVATE
        smtp_proto
        GTest::gtest
        GTest::gtest_main
    )
endif()