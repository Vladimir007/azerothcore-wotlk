CollectSourceFiles(${CMAKE_CURRENT_SOURCE_DIR} PRIVATE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)
CollectIncludeDirectories(${CMAKE_CURRENT_SOURCE_DIR} PUBLIC_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

add_library(common ${PRIVATE_SOURCES})
target_include_directories(common PUBLIC ${CMAKE_BINARY_DIR} ${PUBLIC_INCLUDES} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(common PRIVATE ncore-core-interface PUBLIC boost argon2 sfmt utf8cpp openssl threads jemalloc g3dlib Detour mpq)
set_target_properties(common PROPERTIES FOLDER "server")
target_precompile_headers(common PRIVATE PrecompiledHeaders/commonPCH.h)
