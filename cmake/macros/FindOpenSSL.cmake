macro(_OpenSSL_test_and_find_dependencies ssl_library crypto_library)
  unset(_OpenSSL_extra_static_deps)
  if(("${ssl_library}" MATCHES "\\${CMAKE_STATIC_LIBRARY_SUFFIX}$") OR ("${crypto_library}" MATCHES "\\${CMAKE_STATIC_LIBRARY_SUFFIX}$"))
    set(_OpenSSL_has_dependencies TRUE)
    unset(_OpenSSL_has_dependency_zlib)
    if(OPENSSL_USE_STATIC_LIBS)
      set(_OpenSSL_libs "${_OPENSSL_STATIC_LIBRARIES}")
      set(_OpenSSL_ldflags_other "${_OPENSSL_STATIC_LDFLAGS_OTHER}")
    else()
      set(_OpenSSL_libs "${_OPENSSL_LIBRARIES}")
      set(_OpenSSL_ldflags_other "${_OPENSSL_LDFLAGS_OTHER}")
    endif()
    if(_OpenSSL_libs)
      unset(_OpenSSL_has_dependency_dl)
      foreach(_OPENSSL_DEP_LIB IN LISTS _OpenSSL_libs)
        if (_OPENSSL_DEP_LIB STREQUAL "ssl" OR _OPENSSL_DEP_LIB STREQUAL "crypto")
          # ignoring: these are the targets
        elseif(_OPENSSL_DEP_LIB STREQUAL CMAKE_DL_LIBS)
          set(_OpenSSL_has_dependency_dl TRUE)
        elseif(_OPENSSL_DEP_LIB STREQUAL "z")
          find_package(ZLIB)
          set(_OpenSSL_has_dependency_zlib TRUE)
        else()
          list(APPEND _OpenSSL_extra_static_deps "${_OPENSSL_DEP_LIB}")
        endif()
      endforeach()
      unset(_OPENSSL_DEP_LIB)
    else()
      set(_OpenSSL_has_dependency_dl TRUE)
    endif()
    if(_OpenSSL_ldflags_other)
      unset(_OpenSSL_has_dependency_threads)
      foreach(_OPENSSL_DEP_LDFLAG IN LISTS _OpenSSL_ldflags_other)
        if (_OPENSSL_DEP_LDFLAG STREQUAL "-pthread")
          set(_OpenSSL_has_dependency_threads TRUE)
          find_package(Threads)
        endif()
      endforeach()
      unset(_OPENSSL_DEP_LDFLAG)
    else()
      set(_OpenSSL_has_dependency_threads TRUE)
      find_package(Threads)
    endif()
    unset(_OpenSSL_libs)
    unset(_OpenSSL_ldflags_other)
  else()
    set(_OpenSSL_has_dependencies FALSE)
  endif()
endmacro()

function(_OpenSSL_add_dependencies libraries_var)
  if(_OpenSSL_has_dependency_zlib)
    list(APPEND ${libraries_var} ${ZLIB_LIBRARY})
  endif()
  if(_OpenSSL_has_dependency_threads)
    list(APPEND ${libraries_var} ${CMAKE_THREAD_LIBS_INIT})
  endif()
  if(_OpenSSL_has_dependency_dl)
    list(APPEND ${libraries_var} ${CMAKE_DL_LIBS})
  endif()
  list(APPEND ${libraries_var} ${_OpenSSL_extra_static_deps})
  set(${libraries_var} ${${libraries_var}} PARENT_SCOPE)
endfunction()

function(_OpenSSL_target_add_dependencies target)
  if(_OpenSSL_has_dependencies)
    if(_OpenSSL_has_dependency_zlib)
      set_property( TARGET ${target} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ZLIB::ZLIB )
    endif()
    if(_OpenSSL_has_dependency_threads)
      set_property( TARGET ${target} APPEND PROPERTY INTERFACE_LINK_LIBRARIES Threads::Threads)
    endif()
    if(_OpenSSL_has_dependency_dl)
      set_property( TARGET ${target} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${CMAKE_DL_LIBS} )
    endif()
    if(_OpenSSL_extra_static_deps)
      set_property( TARGET ${target} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${_OpenSSL_extra_static_deps})
    endif()
  endif()
endfunction()

find_package(PkgConfig QUIET)
pkg_check_modules(_OPENSSL QUIET openssl)

# Support preference of static libs by adjusting CMAKE_FIND_LIBRARY_SUFFIXES
if(OPENSSL_USE_STATIC_LIBS)
  set(_openssl_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
  set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()

set(_OPENSSL_FIND_PATH_SUFFIX "include")

if (OPENSSL_ROOT_DIR OR NOT "$ENV{OPENSSL_ROOT_DIR}" STREQUAL "")
  set(_OPENSSL_ROOT_HINTS HINTS ${OPENSSL_ROOT_DIR} ENV OPENSSL_ROOT_DIR)
  set(_OPENSSL_ROOT_PATHS NO_DEFAULT_PATH)
endif ()

if(HOMEBREW_PREFIX)
  list(APPEND _OPENSSL_ROOT_HINTS "${HOMEBREW_PREFIX}/opt/openssl@3")
endif()

set(_OPENSSL_ROOT_HINTS_AND_PATHS ${_OPENSSL_ROOT_HINTS} ${_OPENSSL_ROOT_PATHS})

find_path(OPENSSL_INCLUDE_DIR
  NAMES openssl/ssl.h ${_OPENSSL_ROOT_HINTS_AND_PATHS}
  HINTS ${_OPENSSL_INCLUDEDIR} ${_OPENSSL_INCLUDE_DIRS}
  PATH_SUFFIXES ${_OPENSSL_FIND_PATH_SUFFIX}
)

find_library(OPENSSL_SSL_LIBRARY
  NAMES ssl${_OPENSSL_NAME_POSTFIX} ssleay32 ssleay32MD
  NAMES_PER_DIR ${_OPENSSL_ROOT_HINTS_AND_PATHS}
  HINTS ${_OPENSSL_LIBDIR} ${_OPENSSL_LIBRARY_DIRS}
  PATH_SUFFIXES lib lib64
)

find_library(OPENSSL_CRYPTO_LIBRARY
  NAMES crypto${_OPENSSL_NAME_POSTFIX}
  NAMES_PER_DIR ${_OPENSSL_ROOT_HINTS_AND_PATHS}
  HINTS ${_OPENSSL_LIBDIR} ${_OPENSSL_LIBRARY_DIRS}
  PATH_SUFFIXES lib lib64
)

mark_as_advanced(OPENSSL_CRYPTO_LIBRARY OPENSSL_SSL_LIBRARY)

set(OPENSSL_SSL_LIBRARIES ${OPENSSL_SSL_LIBRARY})
set(OPENSSL_CRYPTO_LIBRARIES ${OPENSSL_CRYPTO_LIBRARY})
set(OPENSSL_LIBRARIES ${OPENSSL_SSL_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARIES} )
_OpenSSL_test_and_find_dependencies("${OPENSSL_SSL_LIBRARY}" "${OPENSSL_CRYPTO_LIBRARY}")
if(_OpenSSL_has_dependencies)
  _OpenSSL_add_dependencies( OPENSSL_SSL_LIBRARIES )
  _OpenSSL_add_dependencies( OPENSSL_CRYPTO_LIBRARIES )
  _OpenSSL_add_dependencies( OPENSSL_LIBRARIES )
endif()

function(from_hex HEX DEC)
  string(TOUPPER "${HEX}" HEX)
  set(_res 0)
  string(LENGTH "${HEX}" _strlen)

  while (_strlen GREATER 0)
    math(EXPR _res "${_res} * 16")
    string(SUBSTRING "${HEX}" 0 1 NIBBLE)
    string(SUBSTRING "${HEX}" 1 -1 HEX)
    if (NIBBLE STREQUAL "A")
      math(EXPR _res "${_res} + 10")
    elseif (NIBBLE STREQUAL "B")
      math(EXPR _res "${_res} + 11")
    elseif (NIBBLE STREQUAL "C")
      math(EXPR _res "${_res} + 12")
    elseif (NIBBLE STREQUAL "D")
      math(EXPR _res "${_res} + 13")
    elseif (NIBBLE STREQUAL "E")
      math(EXPR _res "${_res} + 14")
    elseif (NIBBLE STREQUAL "F")
      math(EXPR _res "${_res} + 15")
    else()
      math(EXPR _res "${_res} + ${NIBBLE}")
    endif()

    string(LENGTH "${HEX}" _strlen)
  endwhile()

  set(${DEC} ${_res} PARENT_SCOPE)
endfunction()

if(OPENSSL_INCLUDE_DIR AND EXISTS "${OPENSSL_INCLUDE_DIR}/openssl/opensslv.h")
    file(STRINGS "${OPENSSL_INCLUDE_DIR}/openssl/opensslv.h" OPENSSL_VERSION_STR
         REGEX "^#[\t ]*define[\t ]+OPENSSL_VERSION_STR[\t ]+\"([0-9])+\\.([0-9])+\\.([0-9])+\".*")
    string(REGEX REPLACE "^.*OPENSSL_VERSION_STR[\t ]+\"([0-9]+\\.[0-9]+\\.[0-9]+)\".*$"
           "\\1" OPENSSL_VERSION_STR "${OPENSSL_VERSION_STR}")

    set(OPENSSL_VERSION "${OPENSSL_VERSION_STR}")

    unset(OPENSSL_VERSION_STR)
endif ()

foreach(_comp IN LISTS OpenSSL_FIND_COMPONENTS)
  if(_comp STREQUAL "Crypto")
    if(EXISTS "${OPENSSL_INCLUDE_DIR}" AND
        (EXISTS "${OPENSSL_CRYPTO_LIBRARY}" OR
        EXISTS "${LIB_EAY_LIBRARY_DEBUG}" OR
        EXISTS "${LIB_EAY_LIBRARY_RELEASE}")
    )
      set(OpenSSL_${_comp}_FOUND TRUE)
    else()
      set(OpenSSL_${_comp}_FOUND FALSE)
    endif()
  elseif(_comp STREQUAL "SSL")
    if(EXISTS "${OPENSSL_INCLUDE_DIR}" AND
        (EXISTS "${OPENSSL_SSL_LIBRARY}" OR
        EXISTS "${SSL_EAY_LIBRARY_DEBUG}" OR
        EXISTS "${SSL_EAY_LIBRARY_RELEASE}")
    )
      set(OpenSSL_${_comp}_FOUND TRUE)
    else()
      set(OpenSSL_${_comp}_FOUND FALSE)
    endif()
  else()
    message(WARNING "${_comp} is not a valid OpenSSL component")
    set(OpenSSL_${_comp}_FOUND FALSE)
  endif()
endforeach()
unset(_comp)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenSSL
  REQUIRED_VARS OPENSSL_CRYPTO_LIBRARY OPENSSL_INCLUDE_DIR
  VERSION_VAR OPENSSL_VERSION
  HANDLE_COMPONENTS
  FAIL_MESSAGE "Could NOT find OpenSSL, try to set the path to OpenSSL root folder in the system variable OPENSSL_ROOT_DIR"
)

mark_as_advanced(OPENSSL_INCLUDE_DIR)

if(OPENSSL_FOUND)
  if(NOT TARGET OpenSSL::Crypto AND
      (EXISTS "${OPENSSL_CRYPTO_LIBRARY}" OR
        EXISTS "${LIB_EAY_LIBRARY_DEBUG}" OR
        EXISTS "${LIB_EAY_LIBRARY_RELEASE}")
      )
    add_library(OpenSSL::Crypto UNKNOWN IMPORTED)
    set_target_properties(OpenSSL::Crypto PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")
    if(EXISTS "${OPENSSL_CRYPTO_LIBRARY}")
      set_target_properties(OpenSSL::Crypto PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${OPENSSL_CRYPTO_LIBRARY}")
    endif()
    if(EXISTS "${LIB_EAY_LIBRARY_RELEASE}")
      set_property(TARGET OpenSSL::Crypto APPEND PROPERTY
        IMPORTED_CONFIGURATIONS RELEASE)
      set_target_properties(OpenSSL::Crypto PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
        IMPORTED_LOCATION_RELEASE "${LIB_EAY_LIBRARY_RELEASE}")
    endif()
    if(EXISTS "${LIB_EAY_LIBRARY_DEBUG}")
      set_property(TARGET OpenSSL::Crypto APPEND PROPERTY
        IMPORTED_CONFIGURATIONS DEBUG)
      set_target_properties(OpenSSL::Crypto PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
        IMPORTED_LOCATION_DEBUG "${LIB_EAY_LIBRARY_DEBUG}")
    endif()
    _OpenSSL_target_add_dependencies(OpenSSL::Crypto)
  endif()

  if(NOT TARGET OpenSSL::SSL AND
      (EXISTS "${OPENSSL_SSL_LIBRARY}" OR
        EXISTS "${SSL_EAY_LIBRARY_DEBUG}" OR
        EXISTS "${SSL_EAY_LIBRARY_RELEASE}")
      )
    add_library(OpenSSL::SSL UNKNOWN IMPORTED)
    set_target_properties(OpenSSL::SSL PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")
    if(EXISTS "${OPENSSL_SSL_LIBRARY}")
      set_target_properties(OpenSSL::SSL PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${OPENSSL_SSL_LIBRARY}")
    endif()
    if(EXISTS "${SSL_EAY_LIBRARY_RELEASE}")
      set_property(TARGET OpenSSL::SSL APPEND PROPERTY
        IMPORTED_CONFIGURATIONS RELEASE)
      set_target_properties(OpenSSL::SSL PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
        IMPORTED_LOCATION_RELEASE "${SSL_EAY_LIBRARY_RELEASE}")
    endif()
    if(EXISTS "${SSL_EAY_LIBRARY_DEBUG}")
      set_property(TARGET OpenSSL::SSL APPEND PROPERTY
        IMPORTED_CONFIGURATIONS DEBUG)
      set_target_properties(OpenSSL::SSL PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
        IMPORTED_LOCATION_DEBUG "${SSL_EAY_LIBRARY_DEBUG}")
    endif()
    if(TARGET OpenSSL::Crypto)
      set_target_properties(OpenSSL::SSL PROPERTIES
        INTERFACE_LINK_LIBRARIES OpenSSL::Crypto)
    endif()
    _OpenSSL_target_add_dependencies(OpenSSL::SSL)
  endif()

  if("${OPENSSL_VERSION_MAJOR}.${OPENSSL_VERSION_MINOR}.${OPENSSL_VERSION_FIX}" VERSION_GREATER_EQUAL "0.9.8")
    if(NOT TARGET OpenSSL::applink)
      add_library(OpenSSL::applink INTERFACE IMPORTED)
      set_property(TARGET OpenSSL::applink APPEND
        PROPERTY INTERFACE_SOURCES
          ${_OPENSSL_applink_interface_srcs})
    endif()
  endif()
endif()

# Restore the original find library ordering
if(OPENSSL_USE_STATIC_LIBS)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${_openssl_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
endif()

unset(_OPENSSL_FIND_PATH_SUFFIX)
unset(_OPENSSL_NAME_POSTFIX)
unset(_OpenSSL_extra_static_deps)
unset(_OpenSSL_has_dependency_dl)
unset(_OpenSSL_has_dependency_threads)
unset(_OpenSSL_has_dependency_zlib)
