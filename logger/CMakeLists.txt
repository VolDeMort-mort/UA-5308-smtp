add_library(logger_lib STATIC
	src/Logger.cpp
	src/Filestrategy.cpp
	src/ConsoleStrategy.cpp
)

target_include_directories(logger_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(Threads REQUIRED)
target_link_libraries(logger_lib PRIVATE Threads::Threads)