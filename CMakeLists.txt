cmake_minimum_required(VERSION 3.16)

project(fsm
    VERSION     2.0.0
    LANGUAGES   C
    DESCRIPTION "Finite state machine and task queue for STM32"
    HOMEPAGE_URL "https://github.com/nimaltd/fsm")

option(FSM_BUILD_TESTS "Build the host unit tests" OFF)

# fsm.c needs one header it does not ship: the platform main.h. Point
# FSM_CONFIG_DIR at the folder holding it, normally Core/Inc in a CubeMX
# project. The tests supply a stub instead.
if(FSM_BUILD_TESTS AND NOT DEFINED FSM_CONFIG_DIR)
    set(FSM_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test/stub")
endif()

add_library(fsm STATIC src/fsm.c)
add_library(nimaltd::fsm ALIAS fsm)

target_include_directories(fsm PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

target_compile_features(fsm PUBLIC c_std_11)

if(DEFINED FSM_CONFIG_DIR)
    target_include_directories(fsm PRIVATE ${FSM_CONFIG_DIR})
endif()

if(FSM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
