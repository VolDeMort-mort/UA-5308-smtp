add_library(logger_lib STATIC
	Logger.cpp
	FileStrategy.cpp
	ConsoleStrategy.cpp
)

target_include_directories(logger_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Threads REQUIRED)
target_link_libraries(logger_lib PRIVATE Threads::Threads)