###############################################################################
# CMake functions for compiling reflex and trison sources into C++ as well
# as creating named targets for them and for diff-based tests for them.
###############################################################################

find_program(barf_REFLEX_BINARY reflex DOC "absolute path to reflex binary for building scanner code from reflex sources")
find_program(barf_TRISON_BINARY trison DOC "absolute path to trison binary for building parser code from trison sources")

option(barf_ENABLE_TARGETS_DIR_OVERRIDE "Enable optional override of the targets dir (barf_TARGETS_DIR_OVERRIDE) in invocations of the reflex and trison binaries (see -I option in reflex and trison help messages)." OFF)
if(barf_ENABLE_TARGETS_DIR_OVERRIDE)
    if(NOT DEFINED barf_TARGETS_DIR_OVERRIDE)
        # Set a reasonable default
        set(barf_TARGETS_DIR_OVERRIDE "${barf_DIR}/targets" CACHE FILEPATH "absolute path to barf targets directory (overrides -I option in reflex and trison invocations)")
    endif()
endif()

include(${barf_DIR}/cmake/reflex-rules.cmake)
include(${barf_DIR}/cmake/trison-rules.cmake)
