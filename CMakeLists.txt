cmake_minimum_required(VERSION 3.16)
project(flu-sim-root)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

function(define_option varname default)
    if (NOT DEFINED ${varname})
        set(${varname} ${default} PARENT_SCOPE)
    endif()
endfunction()

define_option(CMAKE_BUILD_TYPE "Debug")

set(FLU_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(flu-sim)