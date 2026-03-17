find_package(Threads REQUIRED)

include(FetchContent)

FetchContent_Declare(
    mini
    GIT_REPOSITORY https://github.com/pulzed/mINI.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(mini)

add_executable(imap_server
    src/ImapMain.cpp
    src/ImapServer.cpp
    src/ImapSession.cpp
    src/ImapParser.cpp
    src/ImapUtils.cpp
    src/ImapCommandDispatcher.cpp
    src/ImapConfig.cpp
)

target_include_directories(imap_server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${mini_SOURCE_DIR}/src)
target_link_libraries(imap_server PRIVATE smtp_proto Threads::Threads logger_lib db_lib threadpool_lib)

add_subdirectory(tests)
