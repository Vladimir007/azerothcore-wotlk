set(Boost_NO_WARN_NEW_VERSIONS ON)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

include (CheckCXXSourceCompiles)

add_definitions(-D_GNU_SOURCE)
find_package(Boost CONFIG REQUIRED COMPONENTS filesystem program_options iostreams regex)

if(NOT Boost_FOUND)
  if(NOT DEFINED ENV{Boost_ROOT} AND NOT DEFINED Boost_DIR AND NOT DEFINED BOOST_ROOT AND NOT DEFINED BOOSTROOT)
    message(FATAL_ERROR "No Boost_ROOT environment variable could be found! Please make sure it is set and the points to your Boost installation.")
  endif()
endif()

add_library(boost INTERFACE)

target_link_libraries(boost INTERFACE ${Boost_LIBRARIES})
target_include_directories(boost INTERFACE ${Boost_INCLUDE_DIRS})

target_compile_definitions(boost
  INTERFACE
    -DBOOST_DATE_TIME_NO_LIB
    -DBOOST_REGEX_NO_LIB
    -DBOOST_CHRONO_NO_LIB
    -DBOOST_SERIALIZATION_NO_LIB
    -DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
    -DBOOST_ASIO_NO_DEPRECATED
    -DBOOST_SYSTEM_USE_UTF8
    -DBOOST_BIND_NO_PLACEHOLDERS)
