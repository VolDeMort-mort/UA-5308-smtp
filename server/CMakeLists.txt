add_executable(smtp_server
    src/SMTPServer.cpp
    src/SMTPServerSession.cpp
    src/server_main.cpp
)

target_include_directories(smtp_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(smtp_server
    PRIVATE
        smtp_proto
)
