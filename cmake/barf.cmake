###############################################################################
# CMake functions for compiling reflex and trison sources into C++ as well
# as creating named targets for them and for diff-based tests for them.
###############################################################################

# Don't use this function directly.  Use one of add_reflex_source, add_reflex_source_dev instead.
function(__add_reflex_source__impl REFLEX_BINARY SOURCE_DIR SOURCE_BASENAME SPECIFY_TARGETS_DIR TARGETS_DIR OUTPUT_DIR)
    if(NOT TARGETS_DIR)
        #message(FATAL_ERROR "TARGETS_DIR not specified")
        execute_process(
            COMMAND
                ${REFLEX_BINARY} -p
            OUTPUT_VARIABLE
                TARGETS_DIR
        )
        string(STRIP ${TARGETS_DIR} TARGETS_DIR)
        if(NOT TARGETS_DIR)
            message(FATAL_ERROR "TARGETS_DIR not specified")
        endif()
    endif()

    if(SPECIFY_TARGETS_DIR)
        # Clear and then set the include dir.
        set(TARGETS_DIR_OPTION -c -I ${TARGETS_DIR})
    else()
        set(TARGETS_DIR_OPTION "")
    endif()

    set(DEPENDENCIES
        ${TARGETS_DIR}/reflex.cpp.header.codespec
        ${TARGETS_DIR}/reflex.cpp.implementation.codespec
        ${TARGETS_DIR}/reflex.cpp.targetspec
    )

    add_custom_command(
        OUTPUT
            ${OUTPUT_DIR}/${SOURCE_BASENAME}.cpp
            ${OUTPUT_DIR}/${SOURCE_BASENAME}.hpp
            ${OUTPUT_DIR}/${SOURCE_BASENAME}.nfa.dot
            ${OUTPUT_DIR}/${SOURCE_BASENAME}.dfa.dot
        COMMAND
            ${REFLEX_BINARY}
            ${TARGETS_DIR_OPTION}
            ${SOURCE_DIR}/${SOURCE_BASENAME}.reflex
            -o ${OUTPUT_DIR}
            --generate-nfa-dot-graph ${SOURCE_BASENAME}.nfa.dot
            --generate-dfa-dot-graph ${SOURCE_BASENAME}.dfa.dot
        # Ideally the .reflex file would be MAIN_DEPENDENCY, but for some reason this causes targets
        # that depend on that file to be built, even if not called for.
        DEPENDS
            ${SOURCE_DIR}/${SOURCE_BASENAME}.reflex
            ${DEPENDENCIES}
    )

    if(DEFINED DOXYGEN_DOT_EXECUTABLE)
        add_custom_command(
            OUTPUT
                ${OUTPUT_DIR}/${SOURCE_BASENAME}.dfa.png
            COMMAND
                ${DOXYGEN_DOT_EXECUTABLE}
                -Tpng
                -o${OUTPUT_DIR}/${SOURCE_BASENAME}.dfa.png
                ${SOURCE_DIR}/${SOURCE_BASENAME}.dfa.dot
            DEPENDS
                ${SOURCE_DIR}/${SOURCE_BASENAME}.dfa.dot
        )
        add_custom_command(
            OUTPUT
                ${OUTPUT_DIR}/${SOURCE_BASENAME}.nfa.png
            COMMAND
                ${DOXYGEN_DOT_EXECUTABLE}
                -Tpng
                -o${OUTPUT_DIR}/${SOURCE_BASENAME}.nfa.png
                ${SOURCE_DIR}/${SOURCE_BASENAME}.nfa.dot
            DEPENDS
                ${SOURCE_DIR}/${SOURCE_BASENAME}.nfa.dot
        )
    else()
        message(STATUS "Not adding rule(s) for generating [graphviz] dot images from reflex-generated file(s) because DOXYGEN_DOT_EXECUTABLE is not defined (this is not a problem unless you want to visualize the reflex-generated state machine(s)).")
    endif()
endfunction()

function(add_reflex_source SOURCE_FILE)
    get_filename_component(SOURCE_FILE_EXT ${SOURCE_FILE} EXT)
    if(NOT (${SOURCE_FILE_EXT} STREQUAL ".reflex"))
        message(FATAL_ERROR "Source file \"${SOURCE_FILE}\" in call to add_reflex_source does not have extension \".reflex\"")
    endif()
    get_filename_component(SOURCE_DIR ${SOURCE_FILE} DIRECTORY)
    get_filename_component(SOURCE_BASENAME ${SOURCE_FILE} NAME_WE)
    __add_reflex_source__impl(${STABLE_REFLEX_BINARY} ${SOURCE_DIR} ${SOURCE_BASENAME} ${SPECIFY_STABLE_BARF_TARGETS_DIR} "${STABLE_BARF_TARGETS_DIR}" ${SOURCE_DIR})

    set(OUTPUT_FILES ${SOURCE_DIR}/${SOURCE_BASENAME}.cpp ${SOURCE_DIR}/${SOURCE_BASENAME}.hpp ${SOURCE_DIR}/${SOURCE_BASENAME}.dfa.dot ${SOURCE_DIR}/${SOURCE_BASENAME}.nfa.dot)
    add_custom_target(${SOURCE_BASENAME} DEPENDS ${OUTPUT_FILES})
    add_custom_target(clean_${SOURCE_BASENAME} COMMAND rm -f ${OUTPUT_FILES} ${SOURCE_DIR}/${SOURCE_BASENAME}.dfa.png ${SOURCE_DIR}/${SOURCE_BASENAME}.nfa.png)

    if(DEFINED DOXYGEN_DOT_EXECUTABLE)
        add_custom_target(${SOURCE_BASENAME}_dfa_png DEPENDS ${SOURCE_DIR}/${SOURCE_BASENAME}.dfa.png)
        add_custom_target(clean_${SOURCE_BASENAME}_dfa_png COMMAND rm -f ${SOURCE_DIR}/${SOURCE_BASENAME}.dfa.png)

        add_custom_target(${SOURCE_BASENAME}_nfa_png DEPENDS ${SOURCE_DIR}/${SOURCE_BASENAME}.nfa.png)
        add_custom_target(clean_${SOURCE_BASENAME}_nfa_png COMMAND rm -f ${SOURCE_DIR}/${SOURCE_BASENAME}.nfa.png)

        add_custom_target(${SOURCE_BASENAME}_dot_png DEPENDS ${SOURCE_BASENAME}_dfa_png ${SOURCE_BASENAME}_nfa_png)
        add_custom_target(clean_${SOURCE_BASENAME}_dot_png DEPENDS clean_${SOURCE_BASENAME}_dfa_png clean_${SOURCE_BASENAME}_nfa_png)
    endif()
endfunction()

function(add_reflex_source_dev SOURCE_FILE)
    get_filename_component(SOURCE_FILE_EXT ${SOURCE_FILE} EXT)
    if(NOT (${SOURCE_FILE_EXT} STREQUAL ".reflex"))
        message(FATAL_ERROR "Source file \"${SOURCE_FILE}\" in call to add_reflex_source_dev does not have extension \".reflex\"")
    endif()
    get_filename_component(SOURCE_DIR ${SOURCE_FILE} DIRECTORY)
    get_filename_component(SOURCE_BASENAME ${SOURCE_FILE} NAME_WE)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/dev)
    __add_reflex_source__impl(${PROJECT_BINARY_DIR}/bin/reflex ${SOURCE_DIR} ${SOURCE_BASENAME} TRUE ${PROJECT_SOURCE_DIR}/targets ${PROJECT_BINARY_DIR}/dev)

    set(OUTPUT_FILES ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.cpp ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.hpp ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.dfa.dot ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.nfa.dot)
    add_custom_target(dev_${SOURCE_BASENAME} DEPENDS ${OUTPUT_FILES})
    add_custom_target(clean_dev_${SOURCE_BASENAME} COMMAND rm -f ${OUTPUT_FILES} ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.dfa.png ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.nfa.png)

    add_custom_target(
            diff_dev_${SOURCE_BASENAME}
        DEPENDS
            ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.cpp
            ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.hpp
        COMMAND
            diff ${SOURCE_DIR}/${SOURCE_BASENAME}.cpp ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.cpp
        COMMAND
            diff ${SOURCE_DIR}/${SOURCE_BASENAME}.hpp ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.hpp
    )

    if(DEFINED DOXYGEN_DOT_EXECUTABLE)
        add_custom_target(dev_${SOURCE_BASENAME}_dfa_png DEPENDS ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.dfa.png)
        add_custom_target(clean_dev_${SOURCE_BASENAME}_dfa_png COMMAND rm -f ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.dfa.png)

        add_custom_target(dev_${SOURCE_BASENAME}_nfa_png DEPENDS ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.nfa.png)
        add_custom_target(clean_dev_${SOURCE_BASENAME}_nfa_png COMMAND rm -f ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.nfa.png)

        add_custom_target(dev_${SOURCE_BASENAME}_dot_png DEPENDS dev_${SOURCE_BASENAME}_dfa_png dev_${SOURCE_BASENAME}_nfa_png)
        add_custom_target(clean_dev_${SOURCE_BASENAME}_dot_png DEPENDS clean_dev_${SOURCE_BASENAME}_dfa_png clean_dev_${SOURCE_BASENAME}_nfa_png)
    endif()
endfunction()

# Don't use this function directly.  Use one of add_trison_source, add_trison_source_dev instead.
function(__add_trison_source__impl TRISON_BINARY SOURCE_DIR SOURCE_BASENAME SPECIFY_TARGETS_DIR TARGETS_DIR OUTPUT_DIR)
    if(NOT TARGETS_DIR)
        #message(FATAL_ERROR "TARGETS_DIR not specified")
        execute_process(
            COMMAND
                ${TRISON_BINARY} -p
            OUTPUT_VARIABLE
                TARGETS_DIR
        )
        string(STRIP ${TARGETS_DIR} TARGETS_DIR)
        if(NOT TARGETS_DIR)
            message(FATAL_ERROR "TARGETS_DIR not specified")
        endif()
    endif()

    if(SPECIFY_TARGETS_DIR)
        # Clear and then set the include dir.
        set(TARGETS_DIR_OPTION -c -I ${TARGETS_DIR})
    else()
        set(TARGETS_DIR_OPTION "")
    endif()

    # TODO: These dependencies should really just be derived from an invocation of trison.
    set(DEPENDENCIES
        ${TARGETS_DIR}/trison.cpp.header.codespec
        ${TARGETS_DIR}/trison.cpp.implementation.codespec
        ${TARGETS_DIR}/trison.cpp.npda.grammar.header.codespec
        ${TARGETS_DIR}/trison.cpp.npda.grammar.implementation.codespec
        ${TARGETS_DIR}/trison.cpp.npda.header.codespec
        ${TARGETS_DIR}/trison.cpp.npda.implementation.codespec
        ${TARGETS_DIR}/trison.cpp.npda.npda.header.codespec
        ${TARGETS_DIR}/trison.cpp.npda.npda.implementation.codespec
        ${TARGETS_DIR}/trison.cpp.targetspec
    )

    add_custom_command(
        OUTPUT
            ${OUTPUT_DIR}/${SOURCE_BASENAME}.cpp
            ${OUTPUT_DIR}/${SOURCE_BASENAME}.hpp
            ${OUTPUT_DIR}/${SOURCE_BASENAME}.npda.dot
            ${OUTPUT_DIR}/${SOURCE_BASENAME}.npda.states
        COMMAND
            ${TRISON_BINARY}
            ${TARGETS_DIR_OPTION}
            ${SOURCE_DIR}/${SOURCE_BASENAME}.trison
            -o ${OUTPUT_DIR}
            --generate-npda-dot-graph ${SOURCE_BASENAME}.npda.dot
            --generate-npda-states-file ${SOURCE_BASENAME}.npda.states
        # Ideally the .trison file would be MAIN_DEPENDENCY, but for some reason this causes targets
        # that depend on that file to be built, even if not called for.
        DEPENDS
            ${SOURCE_DIR}/${SOURCE_BASENAME}.trison
            ${DEPENDENCIES}
    )

    if(DEFINED DOXYGEN_DOT_EXECUTABLE)
        add_custom_command(
            OUTPUT
                ${OUTPUT_DIR}/${SOURCE_BASENAME}.npda.png
            COMMAND
                ${DOXYGEN_DOT_EXECUTABLE}
                -Tpng
                -o${OUTPUT_DIR}/${SOURCE_BASENAME}.npda.png
                ${SOURCE_DIR}/${SOURCE_BASENAME}.npda.dot
            DEPENDS
                ${SOURCE_DIR}/${SOURCE_BASENAME}.npda.dot
        )
    else()
        message(STATUS "Not adding rule(s) for generating [graphviz] dot images from trison-generated file(s) because DOXYGEN_DOT_EXECUTABLE is not defined (this is not a problem unless you want to visualize the trison-generated pushdown automaton(s)).")
    endif()
endfunction()

function(add_trison_source SOURCE_FILE)
    get_filename_component(SOURCE_FILE_EXT ${SOURCE_FILE} EXT)
    if(NOT (${SOURCE_FILE_EXT} STREQUAL ".trison"))
        message(FATAL_ERROR "Source file \"${SOURCE_FILE}\" in call to add_trison_source does not have extension \".trison\"")
    endif()
    get_filename_component(SOURCE_DIR ${SOURCE_FILE} DIRECTORY)
    get_filename_component(SOURCE_BASENAME ${SOURCE_FILE} NAME_WE)
    __add_trison_source__impl(${STABLE_TRISON_BINARY} ${SOURCE_DIR} ${SOURCE_BASENAME} ${SPECIFY_STABLE_BARF_TARGETS_DIR} "${STABLE_BARF_TARGETS_DIR}" ${SOURCE_DIR})

    set(OUTPUT_FILES ${SOURCE_DIR}/${SOURCE_BASENAME}.cpp ${SOURCE_DIR}/${SOURCE_BASENAME}.hpp ${SOURCE_DIR}/${SOURCE_BASENAME}.npda.dot ${SOURCE_DIR}/${SOURCE_BASENAME}.npda.states)
    add_custom_target(${SOURCE_BASENAME} DEPENDS ${OUTPUT_FILES})
    add_custom_target(clean_${SOURCE_BASENAME} COMMAND rm -f ${OUTPUT_FILES})

    if(DEFINED DOXYGEN_DOT_EXECUTABLE)
        add_custom_target(${SOURCE_BASENAME}_npda_png DEPENDS ${SOURCE_DIR}/${SOURCE_BASENAME}.npda.png)
        add_custom_target(clean_${SOURCE_BASENAME}_npda_png COMMAND rm -f ${SOURCE_DIR}/${SOURCE_BASENAME}.npda.png)

        add_custom_target(${SOURCE_BASENAME}_dot_png DEPENDS ${SOURCE_BASENAME}_npda_png)
        add_custom_target(clean_${SOURCE_BASENAME}_dot_png DEPENDS clean_${SOURCE_BASENAME}_npda_png)
    endif()
endfunction()

function(add_trison_source_dev SOURCE_FILE)
    get_filename_component(SOURCE_FILE_EXT ${SOURCE_FILE} EXT)
    if(NOT (${SOURCE_FILE_EXT} STREQUAL ".trison"))
        message(FATAL_ERROR "Source file \"${SOURCE_FILE}\" in call to add_trison_source_dev does not have extension \".trison\"")
    endif()
    get_filename_component(SOURCE_DIR ${SOURCE_FILE} DIRECTORY)
    get_filename_component(SOURCE_BASENAME ${SOURCE_FILE} NAME_WE)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/dev)
    __add_trison_source__impl(${PROJECT_BINARY_DIR}/bin/trison ${SOURCE_DIR} ${SOURCE_BASENAME} TRUE ${PROJECT_SOURCE_DIR}/targets ${PROJECT_BINARY_DIR}/dev)

    set(OUTPUT_FILES ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.cpp ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.hpp ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.npda.dot ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.npda.states)
    add_custom_target(dev_${SOURCE_BASENAME} DEPENDS ${OUTPUT_FILES})
    add_custom_target(clean_dev_${SOURCE_BASENAME} COMMAND rm -f ${OUTPUT_FILES})

    add_custom_target(
            diff_dev_${SOURCE_BASENAME}
        DEPENDS
            ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.cpp
            ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.hpp
        COMMAND
            diff ${SOURCE_DIR}/${SOURCE_BASENAME}.cpp ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.cpp
        COMMAND
            diff ${SOURCE_DIR}/${SOURCE_BASENAME}.hpp ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.hpp
    )

    if(DEFINED DOXYGEN_DOT_EXECUTABLE)
        add_custom_target(dev_${SOURCE_BASENAME}_npda_png DEPENDS ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.npda.png)
        add_custom_target(clean_dev_${SOURCE_BASENAME}_npda_png COMMAND rm -f ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.npda.png)

        add_custom_target(dev_${SOURCE_BASENAME}_dot_png DEPENDS dev_${SOURCE_BASENAME}_npda_png)
        add_custom_target(clean_dev_${SOURCE_BASENAME}_dot_png DEPENDS clean_dev_${SOURCE_BASENAME}_npda_png)
    endif()
endfunction()

