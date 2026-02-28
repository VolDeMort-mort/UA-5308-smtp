add_library(smtp_proto
    src/SMTPClientSocket.cpp
    src/SMTPServerSocket.cpp
    src/SMTPResponseParser.cpp
    src/SMTPCommandParser.cpp
)

target_include_directories(smtp_proto
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(smtp_proto
    PUBLIC
        Boost::system
        OpenSSL::SSL
        OpenSSL::Crypto
)

target_compile_features(smtp_proto PUBLIC cxx_std_17)
