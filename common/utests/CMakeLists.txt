add_executable(common_tests
    StringUtilsTest.cpp
    TimeUtilsTest.cpp)

target_link_libraries(common_tests
    PRIVATE
    common_utils_lib    
    GTest::gtest_main   
)

include(GoogleTest)
gtest_discover_tests(common_tests)