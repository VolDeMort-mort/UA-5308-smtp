find_program(LCOV_PATH NAMES lcov)
find_program(GENHTML_PATH NAMES genhtml)

if(NOT LCOV_PATH)
    message(FATAL_ERROR "lcov not found. Install it with: sudo apt install lcov")
endif()

if(NOT GENHTML_PATH)
    message(FATAL_ERROR "genhtml not found. Install it with: sudo apt install lcov")
endif()

message(STATUS "Found lcov:    ${LCOV_PATH}")
message(STATUS "Found genhtml: ${GENHTML_PATH}")

add_compile_options(-O0 -g -fprofile-update=atomic)

function(target_code_coverage target)
    set_property(GLOBAL APPEND PROPERTY COVERAGE_TARGETS ${target})
endfunction()
