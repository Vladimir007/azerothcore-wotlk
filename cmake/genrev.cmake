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

# User has manually chosen to ignore the git-tests, so throw them a warning.
# This is done EACH compile so they can be alerted about the consequences.

if(NOT BUILDDIR)
  # Workaround for funny MSVC behaviour - this segment is only used when using cmake gui
  set(BUILDDIR ${CMAKE_BINARY_DIR})
endif()

set(rev_date "1970-01-01 00:00:00 +0000")
set(rev_hash "unknown")
set(rev_branch "Archived")
# No valid git commit date, use today
string(TIMESTAMP rev_date_fallback "%Y-%m-%d %H:%M:%S" UTC)

# For package/copyright information we always need a proper date - keep "Archived/1970" for displaying git info but a valid year elsewhere
string(REGEX MATCH "([0-9]+)-([0-9]+)-([0-9]+)" rev_date_fallback_match ${rev_date_fallback})
set(rev_year ${CMAKE_MATCH_1})
set(rev_month ${CMAKE_MATCH_2})
set(rev_day ${CMAKE_MATCH_3})

# Create the actual revision.h file from the above params
if(NOT "${rev_hash_cached}" STREQUAL "${rev_hash}" OR NOT "${rev_branch_cached}" STREQUAL "${rev_branch}" OR NOT EXISTS "${BUILDDIR}/revision.h")
  configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/revision.h.in.cmake"
    "${BUILDDIR}/revision.h"
    @ONLY
  )
  set(rev_hash_cached "${rev_hash}" CACHE INTERNAL "Cached commit-hash")
  set(rev_branch_cached "${rev_branch}" CACHE INTERNAL "Cached branch name")
endif()
