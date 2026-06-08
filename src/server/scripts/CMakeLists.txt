# Creates a list of all script modules and stores it in the given variable.
function(GetScriptModuleList variable)
    file(GLOB module_list RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

    set(${variable})
    foreach(module ${module_list})
        if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${module})
            list(APPEND ${variable} ${module})
        endif()
    endforeach()
    set(${variable} ${${variable}} PARENT_SCOPE)
endfunction()

function(ConfigureScriptLoader SCRIPTLOADER_NAME LOADER_OUT)
  foreach(LOCALE_SCRIPT_MODULE ${ARGN})
    # Replace bad words
    string(REGEX REPLACE - "_" LOCALE_SCRIPT_MODULE ${LOCALE_SCRIPT_MODULE})

    # Determine the loader function ("Add##${NameOfDirectory}##Scripts()")
    set(LOADER_FUNCTION "Add${LOCALE_SCRIPT_MODULE}Scripts()")

    # Generate the function call and the forward declarations
    set(NCORE_SCRIPTS_FORWARD_DECL "${NCORE_SCRIPTS_FORWARD_DECL}void ${LOADER_FUNCTION};\n")

    set(NCORE_SCRIPTS_INVOKE "${NCORE_SCRIPTS_INVOKE} ${LOADER_FUNCTION};\n")
  endforeach()

  set(GENERATED_LOADER ${CMAKE_CURRENT_BINARY_DIR}/gen_scriptloader/${SCRIPTLOADER_NAME}/ScriptLoader.cpp)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ScriptLoader.cpp.in ${GENERATED_LOADER})
  set(${LOADER_OUT} ${GENERATED_LOADER} PARENT_SCOPE)
endfunction()

GetScriptModuleList(SCRIPT_MODULE_LIST)

foreach(SCRIPT_MODULE ${SCRIPT_MODULE_LIST})
    # Uninstall disabled modules
    string(TOLOWER "scripts_${SCRIPT_MODULE}" SCRIPT_MODULE_PROJECT_NAME)
    list(APPEND DISABLED_SCRIPT_MODULE_PROJECTS "${CMAKE_INSTALL_PREFIX}/bin/scripts/lib${SCRIPT_MODULE_PROJECT_NAME}.so")

    # Add the module content to the whole static module
    CollectSourceFiles(${CMAKE_CURRENT_SOURCE_DIR}/${SCRIPT_MODULE} SCRIPTS_SOURCES)
endforeach()

ConfigureScriptLoader("static" SCRIPT_MODULE_LOADER ${SCRIPT_MODULE_LIST})

add_library(scripts STATIC ScriptLoader.h ${SCRIPT_MODULE_LOADER} ${SCRIPTS_SOURCES})
target_link_libraries(scripts PRIVATE ncore-core-interface PUBLIC game-interface)
target_include_directories(scripts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PUBLIC_INCLUDES})
set_target_properties(scripts PROPERTIES FOLDER "scripts")

# Remove all shared libraries in the install directory which
# are contained in the static library already.
if(DISABLED_SCRIPT_MODULE_PROJECTS)
  install(CODE "
    foreach(SCRIPT_TO_UNINSTALL ${DISABLED_SCRIPT_MODULE_PROJECTS})
      if(EXISTS \"\${SCRIPT_TO_UNINSTALL}\")
        message(STATUS \"Uninstalling: \${SCRIPT_TO_UNINSTALL}\")
        file(REMOVE \"\${SCRIPT_TO_UNINSTALL}\")
      endif()
    endforeach()
  ")
endif()
