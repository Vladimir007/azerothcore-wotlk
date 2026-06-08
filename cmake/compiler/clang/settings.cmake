if (CMAKE_C_COMPILER_LAUNCHER STREQUAL "ccache" OR CMAKE_CXX_COMPILER_LAUNCHER STREQUAL "ccache")
  message(STATUS "Clang: disable pch timestamp when ccache and pch enabled")
  # TODO: for ccache https://github.com/ccache/ccache/issues/539
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -fno-pch-timestamp")
endif()

set(CLANG_EXPECTED_VERSION 10.0.0)

if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS CLANG_EXPECTED_VERSION)
  message(FATAL_ERROR "Clang: NordCore requires version ${CLANG_EXPECTED_VERSION} to build but found ${CMAKE_CXX_COMPILER_VERSION}")
else()
  message(STATUS "Clang: Minimum version required is ${CLANG_EXPECTED_VERSION}, found ${CMAKE_CXX_COMPILER_VERSION} - ok!")
endif()

if(WITH_WARNINGS)
  target_compile_options(ncore-warning-interface INTERFACE -W -Wall -Wextra -Winit-self -Wfatal-errors -Wno-mismatched-tags -Woverloaded-virtual)
  message(STATUS "Clang: All warnings enabled")
endif()

# -Wno-narrowing needed to suppress a warning in g3d
target_compile_options(ncore-compile-option-interface INTERFACE -Wno-narrowing)
