include(FetchContent)

FetchContent_Declare(
    libpqxx
    GIT_REPOSITORY https://github.com/jtv/libpqxx.git
    GIT_TAG 8.0.0
)

set(CPACK_PACKAGE_NAME "dummy" CACHE INTERNAL "")
set(CPACK_GENERATOR "TGZ" CACHE INTERNAL "")

set(SKIP_BUILD_TEST ON CACHE BOOL "" FORCE)
set(BUILD_DOC OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(libpqxx)
