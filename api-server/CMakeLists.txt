find_package(Boost REQUIRED COMPONENTS json system)
find_package(OpenSSL REQUIRED)

add_library(api_server_lib
    ApiServer.cpp
    ApiHandler.cpp
)

target_include_directories(api_server_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/database
    ${PROJECT_SOURCE_DIR}/database/Repository
    ${PROJECT_SOURCE_DIR}/database/DAL
    ${PROJECT_SOURCE_DIR}/database/Entity
    ${PROJECT_SOURCE_DIR}/logger/include
    ${PROJECT_SOURCE_DIR}/server/threadpool
)

target_compile_definitions(api_server_lib PRIVATE
    _WIN32_WINNT=0x0601
)

target_link_libraries(api_server_lib PUBLIC
    db_lib
    logger_lib
    threadpool_lib
    Boost::json
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)

add_executable(api_server
    ApiMain.cpp
)

target_compile_definitions(api_server PRIVATE
    _WIN32_WINNT=0x0601
)

target_link_libraries(api_server PRIVATE
    api_server_lib
)

if(SMTP_TESTING)
    add_subdirectory(tests)
endif()