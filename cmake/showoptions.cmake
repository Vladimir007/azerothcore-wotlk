#
# This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#

# output generic information about the core and buildtype chosen
message("")
message("* AzerothCore revision            : ${rev_hash} ${rev_date} (${rev_branch} branch)")
if( UNIX )
  message("* AzerothCore buildtype           : ${CMAKE_BUILD_TYPE}")
endif()
message("")

# output information about installation-directories and locations

message("* Install core to                 : ${CMAKE_INSTALL_PREFIX}")
if( UNIX )
  message("* Install libraries to            : ${LIBSDIR}")
endif()

message("* Install configs to              : ${CONF_DIR}")
add_definitions(-D_CONF_DIR=$<1:"${CONF_DIR}">)

message("")

# Show infomation about the options selected during configuration

if (APPS_BUILD AND (NOT APPS_BUILD STREQUAL "none"))
  message("* Build applications              : Yes (${APPS_BUILD})")
else()
  message("* Build applications              : No")
endif()

if (TOOLS_BUILD AND (NOT TOOLS_BUILD STREQUAL "none"))
  message("* Build tools                     : Yes (${TOOLS_BUILD})")
  add_definitions(-DNO_CORE_FUNCS)
else()
  message("* Build tools                     : No")
endif()

if( WITH_WARNINGS )
  message("* Show all warnings               : Yes")
else()
  message("* Show compile-warnings           : No  (default)")
endif()

if( WIN32 )
  if( USE_MYSQL_SOURCES )
  message("* Use MySQL sourcetree            : Yes (default)")
  else()
  message("* Use MySQL sourcetree            : No")
  endif()
endif( WIN32 )

if ( NOJEM )
  message("")
  message(" *** NOJEM - WARNING!")
  message(" *** jemalloc linking has been disabled!")
  message(" *** Please note that this is for DEBUGGING WITH VALGRIND only!")
  message(" *** DO NOT DISABLE IT UNLESS YOU KNOW WHAT YOU'RE DOING!")
endif()

# Performance optimization options:

if(MSAN)
    message("")
    message(" *** MSAN - WARNING!")
    message(" *** Please note that this is for DEBUGGING WITH MEMORY SANITIZER only!")
    add_definitions(-DMSAN)
endif()

if(UBSAN)
    message("")
    message(" *** UBSAN - WARNING!")
    message(" *** Please note that this is for DEBUGGING WITH UNDEFINED BEHAVIOR SANITIZER only!")
    add_definitions(-DUBSAN)
endif()

if(TSAN)
    message("")
    message(" *** TSAN - WARNING!")
    message(" *** Please note that this is for DEBUGGING WITH THREAD SANITIZER only!")
    add_definitions(-DTSAN -DNO_BUFFERPOOL)
endif()

message("")
