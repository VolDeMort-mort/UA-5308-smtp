add_executable(test_coverage_test test_main.cpp)

target_compile_options(test_coverage_test PRIVATE -O0 -g -fprofile-arcs -ftest-coverage)
target_link_options(test_coverage_test PRIVATE -fprofile-arcs -ftest-coverage)

add_test(NAME coverage_test COMMAND test_coverage_test)