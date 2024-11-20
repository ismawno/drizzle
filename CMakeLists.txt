cmake_minimum_required(VERSION 3.16)
project(flu-sim-root)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist" CACHE STRING "Available build types" FORCE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
endif()

set(TOOLKIT_BUILD_TESTS OFF CACHE BOOL "Build toolkit tests" FORCE)
set(TOOLKIT_BUILD_PERFORMANCE OFF CACHE BOOL "Build toolkit performance tests" FORCE)
set(ONYX_BUILD_DEMOS OFF CACHE BOOL "Build Onyx demos" FORCE)

add_subdirectory(flu-sim)