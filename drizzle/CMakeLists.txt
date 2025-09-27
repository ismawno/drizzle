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
  GIT_TAG v0.6.0
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE)
FetchContent_MakeAvailable(onyx)

FetchContent_Declare(
  argparse
  GIT_REPOSITORY https://github.com/ismawno/argparse.git
  GIT_TAG v3.2.x
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE)
FetchContent_MakeAvailable(argparse)

tkit_register_for_reflection(drizzle SOURCES driz/simulation/settings.hpp
                             driz/simulation/kernel.hpp)
tkit_register_for_yaml_serialization(
  drizzle SOURCES driz/simulation/settings.hpp driz/simulation/kernel.hpp)

target_include_directories(drizzle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                           ${argparse_SOURCE_DIR}/include)
target_link_libraries(drizzle PRIVATE onyx)
target_compile_definitions(drizzle PRIVATE DRIZ_ROOT_PATH="${DRIZZLE_ROOT_PATH}"
                                           DRIZ_VERSION=\"v0.2.1\")

tkit_default_configure(drizzle)
