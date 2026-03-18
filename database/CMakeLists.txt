cmake_minimum_required(VERSION 3.15)
project(UA_5308_smtp)

# Make sure vcpkg toolchain is already used in CMake command
# e.g. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

# --- Find packages ---
find_package(SQLite3 REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)  # For libsodium

# --- Library ---
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

# Include directories
target_include_directories(db_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries
target_link_libraries(db_lib PUBLIC
    SQLite::SQLite3
    unofficial-sodium::sodium
    logger_lib
)

add_subdirectory(tests)