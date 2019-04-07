###############################################################################
# CMake functions for compiling reflex and trison sources into C++ as well
# as creating named targets for them and for diff-based tests for them.
###############################################################################

find_program(REFLEX_BINARY reflex DOC "absolute path to reflex binary")
find_program(TRISON_BINARY trison DOC "absolute path to trison binary")

option(SPECIFY_BARF_TARGETS_DIR "Enable optional specification of an alternate targets include path in invocations of the reflex and trison binaries (see -I option in reflex and trison help messages)." OFF)
if(SPECIFY_BARF_TARGETS_DIR)
    find_path(BARF_TARGETS_DIR DOC "absolute path to barf targets directory")
endif()

include(${barf_DIR}/cmake/reflex-rules.cmake)
include(${barf_DIR}/cmake/trison-rules.cmake)
