##
##  CMakeLists.txt
##  MemoryAllocator
##
##  Created by FireWolf on 2020-09-14.
##

#
# MARK: - Basics
#

cmake_minimum_required(VERSION 3.17)
include(.cmake/Colorful.cmake)

# Set the compiler toolchain if not invoked by CLion
if (DEFINED ENV{CLION_IDE})
    message(STATUS "${BoldYellow}CMakeLists.txt is used by CLion. Will use the toolchain set by CLion.${ColorReset}")
else()
    message(STATUS "${BoldYellow}CMakeLists.txt is used by CMake. Will use the toolchain set by the user.${ColorReset}")
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/CurrentToolchain.cmake)
endif()

# Project Definition
project(MemoryAllocator)

# Set the language standard to `c++20`
set(CMAKE_CXX_STANDARD 20)

# Set the compiler flags
if (MSVC)
    set(CMAKE_C_FLAGS                   "${CMAKE_C_FLAGS} /MP")
    set(CMAKE_C_FLAGS_DEBUG             "${CMAKE_C_FLAGS_DEBUG} /DDEBUG")
    set(CMAKE_C_FLAGS_RELEASE           "${CMAKE_C_FLAGS_RELEASE}")

    set(CMAKE_CXX_FLAGS                 "${CMAKE_CXX_FLAGS} /MP /bigobj")
    set(CMAKE_CXX_FLAGS_DEBUG           "${CMAKE_CXX_FLAGS_DEBUG} /DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE         "${CMAKE_CXX_FLAGS_RELEASE}")
else()
    set(CMAKE_C_FLAGS                   "${CMAKE_C_FLAGS} -Wall -Wextra")
    set(CMAKE_C_FLAGS_DEBUG             "${CMAKE_C_FLAGS_DEBUG} -O0 -DDEBUG")
    set(CMAKE_C_FLAGS_RELEASE           "${CMAKE_C_FLAGS_RELEASE}")

    set(CMAKE_CXX_FLAGS                 "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG           "${CMAKE_CXX_FLAGS_DEBUG} -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE         "${CMAKE_CXX_FLAGS_RELEASE}")
endif()

message(STATUS "${BoldYellow}C Compiler Flags:${ColorReset}\n"
        "\t${BoldGreen}Default:${ColorReset} ${CMAKE_C_FLAGS}\n"
        "\t${BoldGreen}  Debug:${ColorReset} ${CMAKE_C_FLAGS_DEBUG}\n"
        "\t${BoldGreen}Release:${ColorReset} ${CMAKE_C_FLAGS_RELEASE}")

message(STATUS "${BoldYellow}C++ Compiler Flags:${ColorReset}\n"
        "\t${BoldGreen}Default:${ColorReset} ${CMAKE_CXX_FLAGS}\n"
        "\t${BoldGreen}  Debug:${ColorReset} ${CMAKE_CXX_FLAGS_DEBUG}\n"
        "\t${BoldGreen}Release:${ColorReset} ${CMAKE_CXX_FLAGS_RELEASE}")

#
# MARK: - Dependencies
#

if(PROJECT_IS_TOP_LEVEL)
    message(STATUS "${BoldCyan}${PROJECT_NAME} is a top-level project. Will add submodules as dependencies.${ColorReset}")
    add_subdirectory(Dependencies/TinkerLibrary)
else()
    message(STATUS "${BoldMagenta}${PROJECT_NAME} is imported as a subproject. The top-level project will add submodules as dependencies.${ColorReset}")
endif()

#
# MARK: - Targets
#

# Target Definition
set(TARGET              ${PROJECT_NAME})
set(TARGET_TESTS        "${TARGET}Tests")
set(TARGET_PLAYGROUND   "${TARGET}Playground")

# Target: Library
file(GLOB_RECURSE SOURCE_FILES Sources/*.cpp)
add_library(${TARGET} ${SOURCE_FILES})
target_include_directories(${TARGET} PUBLIC Sources)
target_link_libraries(${TARGET} PUBLIC TinkerLibrary)

# Target: Playground
file(GLOB_RECURSE SOURCE_FILES_PLAYGROUND ${TARGET_PLAYGROUND}/*.cpp)
add_executable(${TARGET_PLAYGROUND} ${SOURCE_FILES_PLAYGROUND})
target_link_libraries(${TARGET_PLAYGROUND} PRIVATE ${TARGET})

# Target: Tests
file(GLOB_RECURSE SOURCE_FILES_TESTS ${TARGET_TESTS}/*.cpp)
add_executable(${TARGET_TESTS} ${SOURCE_FILES_TESTS})
target_link_libraries(${TARGET_TESTS} PRIVATE ${TARGET})
