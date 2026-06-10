include(CheckSymbolExists)

add_library(fmt STATIC)

target_sources(fmt
  PRIVATE
    include/fmt/args.h
    include/fmt/base.h
    include/fmt/chrono.h
    include/fmt/color.h
    include/fmt/compile.h
    include/fmt/core.h
    include/fmt/format.h
    include/fmt/format-inl.h
    include/fmt/os.h
    include/fmt/ostream.h
    include/fmt/printf.h
    include/fmt/ranges.h
    include/fmt/std.h
    include/fmt/xchar.h
    src/format.cc
    src/os.cc)

target_include_directories(fmt PUBLIC include)
target_link_libraries(fmt PRIVATE ncore-dependency-interface)
set_target_properties(fmt PROPERTIES FOLDER "deps")
