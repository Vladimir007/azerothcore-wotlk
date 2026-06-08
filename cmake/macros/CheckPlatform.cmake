if(NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
    message(FATAL_ERROR "System is not supported: ${CMAKE_SYSTEM_NAME}")
endif()

if(CMAKE_SIZEOF_VOID_P MATCHES 8)
    message(STATUS "Detected 64-bit platform")
else()
    message(FATAL_ERROR "32-bit platform is not supported")
endif()

if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "amd64|x86_64|AMD64")
    message(FATAL_ERROR "System processor is not supported: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

if(NOT CMAKE_CXX_BYTE_ORDER STREQUAL "LITTLE_ENDIAN")
    message(FATAL_ERROR "This project only supports little-endian platforms.")
endif()

# Set default configuration directory
if(NOT CONF_DIR)
    set(CONF_DIR ${CMAKE_INSTALL_PREFIX}/etc)
    message(STATUS "UNIX: Using default configuration directory")
endif()

message(STATUS "UNIX: Detected compiler: ${CMAKE_C_COMPILER}")
if(CMAKE_C_COMPILER MATCHES "gcc" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
    include(${CMAKE_SOURCE_DIR}/cmake/compiler/gcc/settings.cmake)
elseif(CMAKE_C_COMPILER MATCHES "clang" OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    include(${CMAKE_SOURCE_DIR}/cmake/compiler/clang/settings.cmake)
endif()
