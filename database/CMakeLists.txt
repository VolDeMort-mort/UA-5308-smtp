find_package(SQLite3 REQUIRED)

add_library(db_lib STATIC
    DataBaseManager.cpp
    Entity/Recipient.cpp
    DAL/MessageDAL.cpp
    DAL/FolderDAL.cpp
    DAL/RecipientDAL.cpp
    DAL/UserDAL.cpp
    Repository/MessageRepository.cpp
    Repository/UserRepository.cpp
)
target_include_directories(db_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(db_lib PUBLIC SQLite::SQLite3)
add_subdirectory(tests)