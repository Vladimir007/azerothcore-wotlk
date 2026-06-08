cmake_minimum_required(VERSION 4.2)

message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "Installation path: ${CMAKE_INSTALL_PREFIX}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)

project(NordCore VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

set(CMAKE_SKIP_BUILD_RPATH 0)
set(CMAKE_BUILD_WITH_INSTALL_RPATH 0)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH 1)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # Export compile commands for IDE support

# Set macro-directory
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/macros")

option(WITH_APPS "Build apps" ON)
option(WITH_TOOLS "Build tools" OFF)
option(WITH_WARNINGS "Show all warnings during compile" OFF)

# Build in Release-mode by default if not explicitly set
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

include(ConfigureBaseTargets)
include(CheckPlatform)
include(AutoCollect)
include(cmake/ShowOptions.cmake)

add_subdirectory(deps)
add_subdirectory(src/common)
add_subdirectory(src)
