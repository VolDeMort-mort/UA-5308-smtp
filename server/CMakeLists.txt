find_package(Threads REQUIRED)

add_executable(smtp_server main.cpp)
target_link_libraries(smtp_server PRIVATE smtp_proto Threads::Threads)
