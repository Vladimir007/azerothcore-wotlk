message("")
message("* NordCore buildtype           : ${CMAKE_BUILD_TYPE}")
message("")

message("* Install core to                 : ${CMAKE_INSTALL_PREFIX}")
message("* Install libraries to            : ${CMAKE_INSTALL_PREFIX}/lib")
message("* Install configs to              : ${CONF_DIR}")
add_definitions(-D_CONF_DIR=$<1:"${CONF_DIR}">)

message("")

if (WITH_APPS)
  message("* Build applications              : Yes (default)")
else ()
  message("* Build applications              : No")
endif ()

if (WITH_TOOLS)
  message("* Build tools                     : Yes")
else ()
  message("* Build tools                     : No (default)")
endif ()

if (WITH_WARNINGS)
  message("* Show all warnings               : Yes")
else ()
  message("* Show compile-warnings           : No (default)")
endif ()

message("")
