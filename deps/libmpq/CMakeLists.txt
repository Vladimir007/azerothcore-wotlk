file(GLOB sources libmpq/*.c libmpq/*.h)
add_library(mpq STATIC ${sources})
set_target_properties(mpq PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(mpq
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${WIN_EXTRA_INCLUDE}
    ${CMAKE_SOURCE_DIR}/deps/zlib
    ${CMAKE_SOURCE_DIR}/deps/bzip2)

target_link_libraries(mpq PRIVATE ncore-dependency-interface PUBLIC zlib bzip2)
set_target_properties(mpq PROPERTIES FOLDER "deps")