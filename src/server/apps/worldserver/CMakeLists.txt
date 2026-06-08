CollectSourceFiles(${CMAKE_CURRENT_SOURCE_DIR} WORLDSERVER_PRIVATE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)
CollectIncludeDirectories(${CMAKE_CURRENT_SOURCE_DIR} WORLDSERVER_PUBLIC_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

add_executable(worldserver ${WORLDSERVER_PRIVATE_SOURCES})

target_link_libraries(worldserver PRIVATE ncore-core-interface)
target_link_libraries(worldserver PUBLIC scripts game readline)
set_target_properties(worldserver PROPERTIES LINK_FLAGS "-pthread -lncurses")
target_include_directories(worldserver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(worldserver PUBLIC ${WORLDSERVER_PUBLIC_INCLUDES} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/worldserver)
set_target_properties(worldserver PROPERTIES FOLDER "server")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/worldserver.conf.dist" DESTINATION "${CONF_DIR}")
install(TARGETS worldserver DESTINATION bin)
