file(GLOB_RECURSE sources *.c)

list(REMOVE_ITEM sources ${CMAKE_CURRENT_SOURCE_DIR}/argon2/ref.c)

add_library(argon2 STATIC ${sources})
target_compile_definitions(argon2 PRIVATE -DARGON2_NO_THREADS)
set_target_properties(argon2 PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(argon2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(argon2 PRIVATE ncore-dependency-interface)
set_target_properties(argon2 PROPERTIES FOLDER "deps")
