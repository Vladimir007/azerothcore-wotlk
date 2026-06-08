# Use -std=c++11 instead of -std=gnu++11
set(CXX_EXTENSIONS OFF)

# Enable C++20 support
set(CMAKE_CXX_STANDARD 20)
message(STATUS "Enabled С++20 standard")

# An interface library to make the target com available to other targets
add_library(ncore-compile-option-interface INTERFACE)

# An interface library to make the warnings level available to other targets
# This interface taget is set-up through the platform specific script
add_library(ncore-warning-interface INTERFACE)

# An interface used for all other interfaces
add_library(ncore-default-interface INTERFACE)
target_link_libraries(ncore-default-interface INTERFACE ncore-compile-option-interface)

# An interface used for silencing all warnings
add_library(ncore-no-warning-interface INTERFACE)
target_compile_options(ncore-no-warning-interface INTERFACE -w)

# An interface amalgamation which provides the flags and definitions
# used by the dependency targets.
add_library(ncore-dependency-interface INTERFACE)
target_link_libraries(ncore-dependency-interface INTERFACE ncore-default-interface ncore-no-warning-interface)

# An interface amalgamation which provides the flags and definitions
# used by the core targets.
add_library(ncore-core-interface INTERFACE)
target_link_libraries(ncore-core-interface INTERFACE ncore-default-interface ncore-warning-interface)
