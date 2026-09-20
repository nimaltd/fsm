# Unity is vendored in test/unity. It is third party code, so it is built as its
# own target and is deliberately left out of this project's -Werror.
add_library(unity STATIC unity/unity.c)
target_include_directories(unity PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/unity)

add_executable(test_seq test_seq.c)

target_link_libraries(test_seq PRIVATE seq unity)

# The stubs stand in for the CubeMX main.h and for your seq_config.h.
target_include_directories(test_seq PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stub)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_seq PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(seq PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME seq COMMAND test_seq)
