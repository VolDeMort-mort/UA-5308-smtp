add_library(server_lib
    src/SMTPServer.cpp
    src/SMTPServerSession.cpp
)

target_include_directories(server_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(server_lib
    PUBLIC
        smtp_proto
)

add_executable(smtp_server
    src/server_main.cpp
)

target_link_libraries(smtp_server
    PRIVATE
        server_lib
)

add_custom_command(TARGET smtp_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:smtp_server>/certs
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/certs
        $<TARGET_FILE_DIR:smtp_server>/certs
)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
