cmake_minimum_required(VERSION 3.15)
project(UA_5308_smtp)

# Make sure vcpkg toolchain is already used in CMake command
# e.g. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

# --- Find packages ---
find_package(SQLite3 REQUIRED)

# --- Find libsodium manually ---
find_path(SODIUM_INCLUDE_DIR sodium.h)
find_library(SODIUM_LIBRARY NAMES sodium libsodium)

if(NOT SODIUM_INCLUDE_DIR OR NOT SODIUM_LIBRARY)
    message(FATAL_ERROR "libsodium not found. Please install it or provide SODIUM_INCLUDE_DIR / SODIUM_LIBRARY paths.")
endif()

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
    ${SODIUM_INCLUDE_DIR}       # Add libsodium include
)

# Link libraries
target_link_libraries(db_lib PUBLIC
    SQLite::SQLite3
    ${SODIUM_LIBRARY}            # Link libsodium
    logger_lib
)

# --- Tests ---
add_subdirectory(tests)