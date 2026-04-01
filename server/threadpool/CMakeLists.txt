add_library(threadpool_lib STATIC ThreadPool.cpp)

target_include_directories(threadpool_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(threadpool_lib PUBLIC logger_lib Threads::Threads)

option(SMTP_TEST_THREADPOOL "Build threadpool tests" ${SMTP_TESTING})
if(SMTP_TEST_THREADPOOL)
    add_subdirectory(tests)
endif()
