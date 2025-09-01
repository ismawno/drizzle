cmake_minimum_required(VERSION 3.16)
project(drizzle)

set(SOURCES
    driz/main.cpp
    driz/core/core.cpp
    driz/app/sim_layer.cpp
    driz/app/intro_layer.cpp
    driz/app/visualization.cpp
    driz/app/inspector.cpp
    driz/app/argparse.cpp
    driz/simulation/solver.cpp
    driz/simulation/kernel.cpp
    driz/simulation/lookup.cpp)

add_executable(drizzle ${SOURCES})

include(FetchContent)
FetchContent_Declare(
  onyx
  GIT_REPOSITORY https://github.com/ismawno/onyx.git
  GIT_TAG v0.1.2)
FetchContent_MakeAvailable(onyx)

FetchContent_Declare(
  argparse
  GIT_REPOSITORY https://github.com/p-ranav/argparse.git
  GIT_TAG v3.2)
FetchContent_MakeAvailable(argparse)

tkit_register_for_reflection(drizzle SOURCES driz/simulation/settings.hpp)

target_include_directories(drizzle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                           ${argparse_SOURCE_DIR}/include)
target_link_libraries(drizzle PRIVATE onyx)
target_compile_definitions(drizzle
                           PRIVATE DRIZ_ROOT_PATH="${DRIZZLE_ROOT_PATH}")

if(MSVC)
  tkit_configure_compile_link_options(drizzle FLAGS /W4 /WX)
elseif(UNIX AND NOT APPLE)
  tkit_configure_compile_link_options(
    drizzle
    FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -Wno-changes-meaning)
  tkit_configure_compile_link_options(drizzle FLAGS -Wall -Wextra -Wpedantic
                                      -Werror)
endif()

if(APPLE)
  vkit_add_vulkan_executable_macos_rpath(drizzle)
endif()
