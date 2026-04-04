#
# This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#

if (CMAKE_C_COMPILER_LAUNCHER STREQUAL "ccache" OR CMAKE_CXX_COMPILER_LAUNCHER STREQUAL "ccache")
  message(STATUS "Clang: disable pch timestamp when ccache and pch enabled")
  # TODO: for ccache https://github.com/ccache/ccache/issues/539
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -fno-pch-timestamp")
endif()

set(CLANG_EXPECTED_VERSION 10.0.0)

if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS CLANG_EXPECTED_VERSION)
  message(FATAL_ERROR "Clang: AzerothCore requires version ${CLANG_EXPECTED_VERSION} to build but found ${CMAKE_CXX_COMPILER_VERSION}")
else()
  message(STATUS "Clang: Minimum version required is ${CLANG_EXPECTED_VERSION}, found ${CMAKE_CXX_COMPILER_VERSION} - ok!")
endif()

# This tests for a bug in clang-7 that causes linkage to fail for 64-bit from_chars (in some configurations)
# If the clang requirement is bumped to >= clang-8, you can remove this check, as well as
# the associated ifdef block in src/common/Utilities/StringConvert.h
include(CheckCXXSourceCompiles)

check_cxx_source_compiles("
#include <charconv>
#include <cstdint>
int main()
{
    uint64_t n;
    char const c[] = \"0\";
    std::from_chars(c, c+1, n);
    return static_cast<int>(n);
}
" CLANG_HAVE_PROPER_CHARCONV)

if(WITH_WARNINGS)
  target_compile_options(acore-warning-interface
    INTERFACE
      -W
      -Wall
      -Wextra
      -Winit-self
      -Wfatal-errors
      -Wno-mismatched-tags
      -Woverloaded-virtual)
  message(STATUS "Clang: All warnings enabled")
endif()

if(MSAN)
    target_compile_options(acore-compile-option-interface
            INTERFACE
            -fno-omit-frame-pointer
            -fsanitize=memory
            -fsanitize-memory-track-origins
            -mllvm
            -msan-keep-going=1)

    target_link_options(acore-compile-option-interface
            INTERFACE
            -fno-omit-frame-pointer
            -fsanitize=memory
            -fsanitize-memory-track-origins)

    message(STATUS "Clang: Enabled Memory Sanitizer MSan")
endif()

if(UBSAN)
    target_compile_options(acore-compile-option-interface
            INTERFACE
            -fno-omit-frame-pointer
            -fsanitize=undefined)

    target_link_options(acore-compile-option-interface
            INTERFACE
            -fno-omit-frame-pointer
            -fsanitize=undefined)

    message(STATUS "Clang: Enabled Undefined Behavior Sanitizer UBSan")
endif()

if(TSAN)
    target_compile_options(acore-compile-option-interface
            INTERFACE
            -fno-omit-frame-pointer
            -fsanitize=thread)

    target_link_options(acore-compile-option-interface
            INTERFACE
            -fno-omit-frame-pointer
            -fsanitize=thread)

    message(STATUS "Clang: Enabled Thread Sanitizer TSan")
endif()

# -Wno-narrowing needed to suppress a warning in g3d
# -Wno-deprecated-register is needed to suppress gsoap warnings on Unix systems.
target_compile_options(acore-compile-option-interface
  INTERFACE
    -Wno-narrowing
    -Wno-deprecated-register)
