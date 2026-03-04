add_library(smtp_proto STATIC
    SocketAcceptor.cpp
    SocketConnection.cpp
    SocketConnector.cpp
    ClientSecureChannel.cpp
    SecureChannel.cpp
    ServerSecureChannel.cpp
)
target_include_directories(smtp_proto  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(smtp_proto PUBLIC Boost::headers unofficial-sodium::sodium)
target_link_libraries(smtp_proto PRIVATE logger_lib)


if(BUILD_ENCRYPTION_TESTS)
    find_package(GTest CONFIG REQUIRED)
    add_executable(encryption_test encryption_test.cpp)
    target_link_libraries(encryption_test PRIVATE smtp_proto logger_lib GTest::gtest GTest::gtest_main)
    target_include_directories(encryption_test PRIVATE ${CMAKE_SOURCE_DIR}/logger)  # ← ДОДАЙ
endif()