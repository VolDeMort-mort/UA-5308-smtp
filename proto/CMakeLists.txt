add_library(
    smtp_proto
    STATIC
    SocketAcceptor.cpp
    SocketConnection.cpp
    SocketConnector.cpp
    ClientSecureChannel.cpp
    SecureChannel.cpp
    ServerSecureChannel.cpp
)

target_include_directories(smtp_proto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
    smtp_proto
    PUBLIC Boost::headers Sodium::Sodium logger_lib db_lib
)

option(SMTP_TEST_PROTO "Build proto encryption tests" ${SMTP_TESTING})
if(SMTP_TEST_PROTO)
    add_subdirectory(tests)
endif()
