CollectSourceFiles(${CMAKE_CURRENT_SOURCE_DIR} AUTHSERVER_PRIVATE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)
CollectIncludeDirectories(${CMAKE_CURRENT_SOURCE_DIR} AUTHSERVER_PUBLIC_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

add_executable(authserver ${AUTHSERVER_PRIVATE_SOURCES})
target_link_libraries(authserver PRIVATE ncore-core-interface PUBLIC shared)
target_include_directories(authserver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(authserver PUBLIC ${AUTHSERVER_PUBLIC_INCLUDES} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/authserver)
set_target_properties(authserver PROPERTIES FOLDER "server")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/authserver.conf.dist" DESTINATION "${CONF_DIR}")
install(TARGETS authserver DESTINATION bin)
