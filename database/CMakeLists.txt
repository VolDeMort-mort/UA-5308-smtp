find_package(SQLite3 REQUIRED)

add_library(db_lib STATIC
    DataBaseManager.cpp
    Entity/Recipient.cpp
    DAL/UserDAL.cpp
    DAL/FolderDAL.cpp
    DAL/MessageDAL.cpp
    DAL/RecipientDAL.cpp
    Repository/UserRepository.cpp
    Repository/MessageRepository.cpp
)

target_include_directories(db_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(db_lib 
    PUBLIC 
        SQLite::SQLite3
        unofficial-sodium::sodium
)

add_subdirectory(tests)