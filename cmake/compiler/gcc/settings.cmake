set(GCC_EXPECTED_VERSION 8.0.0)

if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS GCC_EXPECTED_VERSION)
  message(FATAL_ERROR "GCC: This project requires version ${GCC_EXPECTED_VERSION} to build but found ${CMAKE_CXX_COMPILER_VERSION}")
else()
  message(STATUS "GCC: Minimum version required is ${GCC_EXPECTED_VERSION}, found ${CMAKE_CXX_COMPILER_VERSION} - ok!")
endif()

target_compile_definitions(ncore-compile-option-interface INTERFACE -DHAVE_SSE2 -D__SSE2__)

if (WITH_WARNINGS)
  target_compile_options(ncore-warning-interface INTERFACE -W -Wall -Wextra -Winit-self -Winvalid-pch -Wfatal-errors -Woverloaded-virtual)
  message(STATUS "GCC: All warnings enabled")
endif ()
