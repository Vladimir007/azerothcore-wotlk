set(SCRIPTS_AVAILABLE_OPTIONS static)
set(MODULES_AVAILABLE_OPTIONS none static dynamic)
set(BUILD_APPS_AVAILABLE_OPTIONS none all auth-only world-only)
set(BUILD_TOOLS_AVAILABLE_OPTIONS none all db-only maps-only)

set(SCRIPTS "static" CACHE STRING "Build core with scripts")
set(MODULES "static" CACHE STRING "Build core with modules")

set(APPS_BUILD "all" CACHE STRING "Build list for applications")
set(TOOLS_BUILD "none" CACHE STRING "Build list for tools")

set_property(CACHE SCRIPTS PROPERTY STRINGS ${SCRIPTS_AVAILABLE_OPTIONS})
set_property(CACHE MODULES PROPERTY STRINGS ${MODULES_AVAILABLE_OPTIONS})
set_property(CACHE APPS_BUILD PROPERTY STRINGS ${BUILD_APPS_AVAILABLE_OPTIONS})
set_property(CACHE TOOLS_BUILD PROPERTY STRINGS ${BUILD_TOOLS_AVAILABLE_OPTIONS})

# Log a fatal error when the value of the APPS_BUILD variable isn't a valid option.
if(APPS_BUILD)
  list(FIND BUILD_APPS_AVAILABLE_OPTIONS "${APPS_BUILD}" BUILD_APPS_INDEX)
  if(${BUILD_APPS_INDEX} EQUAL -1)
    message(FATAL_ERROR "The value (${APPS_BUILD}) of your APPS_BUILD variable is invalid! "
                        "Allowed values are: ${BUILD_APPS_AVAILABLE_OPTIONS}. Set default")
  endif()
endif()

# Log a fatal error when the value of the TOOLS_BUILD variable isn't a valid option.
if(TOOLS_BUILD)
  list(FIND BUILD_TOOLS_AVAILABLE_OPTIONS "${TOOLS_BUILD}" BUILD_TOOLS_INDEX)
  if(${BUILD_TOOLS_INDEX} EQUAL -1)
    message(FATAL_ERROR "The value (${TOOLS_BUILD}) of your TOOLS_BUILD variable is invalid! "
                        "Allowed values are: ${BUILD_TOOLS_AVAILABLE_OPTIONS}. Set default")
  endif()
endif()

# Build a list of all applications when -DBUILD_APPS="custom" is selected
GetApplicationsList(APPLICATIONS_BUILD_LIST)
foreach(APPLICATION_BUILD_NAME ${APPLICATIONS_BUILD_LIST})
  ApplicationNameToVariable(${APPLICATION_BUILD_NAME} APPLICATION_BUILD_VARIABLE)
  set(${APPLICATION_BUILD_VARIABLE} "default" CACHE STRING "Enable build the ${APPLICATION_BUILD_NAME} application.")
  set_property(CACHE ${APPLICATION_BUILD_VARIABLE} PROPERTY STRINGS default enabled disabled)
endforeach()

# Build a list of all applications when -DBUILD_TOOLS="custom" is selected
GetToolsList(TOOLS_BUILD_LIST)
foreach(TOOL_BUILD_NAME ${TOOLS_BUILD_LIST})
  ToolNameToVariable(${TOOL_BUILD_NAME} TOOL_BUILD_VARIABLE)
  set(${TOOL_BUILD_VARIABLE} "default" CACHE STRING "Enable build the ${TOOL_BUILD_NAME} tool.")
  set_property(CACHE ${TOOL_BUILD_VARIABLE} PROPERTY STRINGS default enabled disabled)
endforeach()

option(WITH_WARNINGS "Show all warnings during compile" 0)

CheckApplicationsBuildList()
CheckToolsBuildList()
