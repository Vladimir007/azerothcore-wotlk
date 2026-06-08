CollectSourceFiles(${CMAKE_CURRENT_SOURCE_DIR} PRIVATE_SOURCES)
CollectIncludeDirectories(${CMAKE_CURRENT_SOURCE_DIR} PUBLIC_INCLUDES)

add_library(shared ${PRIVATE_SOURCES})
target_include_directories(shared PUBLIC ${PUBLIC_INCLUDES} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(shared PRIVATE ncore-core-interface PUBLIC database)
set_target_properties(shared PROPERTIES FOLDER "server")
