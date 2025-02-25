cmake_minimum_required(VERSION 3.16)
project(flu-sim)

set(SOURCES
    flu/main.cpp
    flu/core/core.cpp
    flu/app/sim_layer.cpp
    flu/app/intro_layer.cpp
    flu/app/visualization.cpp
    flu/app/inspector.cpp
    flu/app/argparse.cpp
    flu/simulation/solver.cpp
    flu/simulation/kernel.cpp
    flu/simulation/lookup.cpp
)

add_executable(flu-sim ${SOURCES})


include(FetchContent)
FetchContent_Declare(
    onyx
    GIT_REPOSITORY https://github.com/ismawno/onyx.git
    GIT_TAG main
)
FetchContent_MakeAvailable(onyx)

FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG v3.2
)
FetchContent_MakeAvailable(argparse)

tkit_register_for_reflection(flu-sim SOURCES flu/simulation/settings.hpp)

target_include_directories(flu-sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${argparse_SOURCE_DIR}/include)
target_link_libraries(flu-sim PRIVATE onyx)
target_compile_definitions(flu-sim PRIVATE FLU_ROOT_PATH="${FLU_ROOT_PATH}")

if (MSVC)
    tkit_configure_compile_link_options(flu-sim FLAGS /W4 /WX)
else()
    tkit_configure_compile_link_options(flu-sim FLAGS -Wall -Wextra -Wpedantic -Werror)
endif()
