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

add_custom_command(TARGET smtp_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:smtp_server>/certs
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/certs
        $<TARGET_FILE_DIR:smtp_server>/certs
)

