set(TOOLS_BUILD_LIST mapExtractor mMapsGenerator vMapExtractor vMapAssembler)

foreach(TOOL_NAME ${TOOLS_BUILD_LIST})
  set(SOURCE_TOOL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${TOOL_NAME})

  unset(TOOL_PRIVATE_SOURCES)
  unset(TOOL_PUBLIC_INCLUDES)
  CollectSourceFiles(${SOURCE_TOOL_PATH} TOOL_PRIVATE_SOURCES)
  CollectIncludeDirectories(${SOURCE_TOOL_PATH} TOOL_PUBLIC_INCLUDES)

  add_executable(${TOOL_NAME} ${TOOL_PRIVATE_SOURCES})
  target_link_libraries(${TOOL_NAME} PRIVATE ncore-dependency-interface)
  target_link_libraries(${TOOL_NAME} PUBLIC common shared zlib Recast fkYAML)
  target_include_directories(${TOOL_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${TOOL_PUBLIC_INCLUDES} ${CMAKE_SOURCE_DIR}/src)
  target_include_directories(${TOOL_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${TOOL_NAME})
  set_target_properties(${TOOL_NAME} PROPERTIES FOLDER "tools")

  install(TARGETS ${TOOL_NAME} DESTINATION bin)

  if (${TOOL_NAME} STREQUAL "mMapsGenerator")
    install(FILES ${SOURCE_TOOL_PATH}/mMaps-config.yaml DESTINATION bin)
  endif()
endforeach()
