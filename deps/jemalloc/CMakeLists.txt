include(CheckSymbolExists)
check_symbol_exists(MADV_FREE "sys/mman.h" HAVE_MADV_FREE)

set(JEM_SIZEDEF 3)
set(JEM_TLSMODEL "__attribute__\(\(tls_model\(\"initial-exec\"\)\)\)")
set(JEM_VADDRBITS 48)

if (HAVE_MADV_FREE)
  set(JEM_MADFREE_DEF "#define")
else()
  set(JEM_MADFREE_DEF "#undef")
endif()

set(JEM_CPU_SPINWAIT "__asm__ volatile\(\"pause\"\)")
set(JEM_HAVE_CPU_SPINWAIT 1)

# Create the header, so we can use it
configure_file(
  "${CMAKE_SOURCE_DIR}/deps/jemalloc/jemalloc_internal_defs.h.in.cmake"
  "${CMAKE_BINARY_DIR}/jemalloc_internal_defs.h"
  @ONLY
)

# Done, let's continue
set(jemalloc_STAT_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/arena.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/background_thread.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/base.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/bin.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/bitmap.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ckh.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ctl.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/div.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/extent.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/extent_dss.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/extent_mmap.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/hash.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/hook.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/jemalloc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/jemalloc_cpp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/large.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/log.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/malloc_io.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mutex.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mutex_pool.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/nstime.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pages.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/prng.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/prof.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/rtree.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/safety_check.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/stats.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sz.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/tcache.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_hooks.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ticker.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/tsd.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/witness.c
)

add_library(jemalloc STATIC ${jemalloc_STAT_SRC})
target_include_directories(jemalloc PRIVATE ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(jemalloc PUBLIC -DNO_BUFFERPOOL PRIVATE -D_GNU_SOURCE -D_REENTRAN)
target_link_libraries(jemalloc PRIVATE ncore-dependency-interface PUBLIC threads ${CMAKE_DL_LIBS})
set_target_properties(jemalloc PROPERTIES FOLDER "deps")
