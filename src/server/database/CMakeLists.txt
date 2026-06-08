CollectSourceFiles(${CMAKE_CURRENT_SOURCE_DIR} PRIVATE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)
CollectIncludeDirectories(${CMAKE_CURRENT_SOURCE_DIR} PUBLIC_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

add_library(database ${PRIVATE_SOURCES})
target_include_directories(database PUBLIC ${PUBLIC_INCLUDES} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(database PRIVATE ncore-core-interface libpqxx::pqxx PUBLIC common)
set_target_properties(database PROPERTIES FOLDER "server")
