include(CheckSymbolExists)

set(strtod_l_headers stdlib.h)

check_symbol_exists(strtod_l "${strtod_l_headers}" HAVE_STRTOD_L)

function(add_headers VAR)
  set(headers ${${VAR}})
  foreach (header ${ARGN})
    set(headers ${headers} include/fmt/${header})
  endforeach()
  set(${VAR} ${headers} PARENT_SCOPE)
endfunction()

# Define the fmt library, its includes and the needed defines.
add_headers(FMT_HEADERS
    args.h
    chrono.h
    color.h
    compile.h
    core.h
    format.h
    format-inl.h
    os.h
    ostream.h
    printf.h
    ranges.h
    std.h
    xchar.h)

set(FMT_SOURCES src/format.cc src/os.cc)

add_library(fmt STATIC ${FMT_SOURCES} ${FMT_HEADERS})

if (HAVE_STRTOD_L)
  target_compile_definitions(fmt PUBLIC FMT_LOCALE)
endif()

# Define FMT_CONSTEVAL for current using in c++20
target_compile_definitions(fmt PUBLIC FMT_CONSTEVAL=)
target_include_directories(fmt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(fmt PRIVATE ncore-dependency-interface)
set_target_properties(fmt PROPERTIES FOLDER "deps")