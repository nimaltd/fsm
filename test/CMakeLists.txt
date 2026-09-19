add_executable(test_fsm test_fsm.c)

target_link_libraries(test_fsm PRIVATE fsm)

# The stubs stand in for the CubeMX main.h and for your fsm_config.h.
target_include_directories(test_fsm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stub)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_fsm PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(fsm PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME fsm COMMAND test_fsm)
