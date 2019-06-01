###############################################################################
# CMake functions for compiling trison sources into C++ as well as creating
# named targets for them.
###############################################################################

# Don't use this function directly.  Use trison_add_source instead.
function(__trison_add_source__impl TRISON_BINARY SOURCE_DIR SOURCE_BASENAME SPECIFY_TARGETS_DIR TARGETS_DIR OUTPUT_DIR FORCE_TARGET_NAME BINARY_DEPENDENCY)
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
        ${BINARY_DEPENDENCY}
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

    # This target forces the build to happen -- command should be identical to the actual build rule above.
    add_custom_target(
        ${FORCE_TARGET_NAME}
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
    set_property(TARGET ${FORCE_TARGET_NAME} PROPERTY EXCLUDE_FROM_ALL TRUE)

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

# SOURCE_FILE is the .trison file.  TARGET_NAME is the target that can be invoked to build this particular parser.
function(trison_add_source SOURCE_FILE OUTPUT_DIR TARGET_NAME)
    if(NOT DEFINED barf_TRISON_BINARY)
        message(FATAL_ERROR "Must specify barf_TRISON_BINARY")
    endif()
    if(NOT DEFINED barf_ENABLE_TARGETS_DIR_OVERRIDE)
        message(FATAL_ERROR "Must specify barf_ENABLE_TARGETS_DIR_OVERRIDE (a bool option)")
    endif()
    if(barf_ENABLE_TARGETS_DIR_OVERRIDE)
        if(NOT DEFINED barf_TARGETS_DIR_OVERRIDE)
            message(FATAL_ERROR "If barf_ENABLE_TARGETS_DIR_OVERRIDE is set, then must specify barf_TARGETS_DIR_OVERRIDE (a bool option)")
        endif()
    endif()

    get_filename_component(SOURCE_FILE_EXT ${SOURCE_FILE} EXT)
    if(NOT (${SOURCE_FILE_EXT} STREQUAL ".trison"))
        message(FATAL_ERROR "Source file \"${SOURCE_FILE}\" in call to trison_add_source does not have extension \".trison\"")
    endif()
    get_filename_component(SOURCE_DIR ${SOURCE_FILE} DIRECTORY)
    get_filename_component(SOURCE_BASENAME ${SOURCE_FILE} NAME_WE)
    __trison_add_source__impl(${barf_TRISON_BINARY} ${SOURCE_DIR} ${SOURCE_BASENAME} ${barf_ENABLE_TARGETS_DIR_OVERRIDE} "${barf_TARGETS_DIR_OVERRIDE}" ${OUTPUT_DIR} force_${TARGET_NAME} ${barf_TRISON_BINARY})

    set(OUTPUT_FILES ${SOURCE_DIR}/${SOURCE_BASENAME}.cpp ${SOURCE_DIR}/${SOURCE_BASENAME}.hpp ${SOURCE_DIR}/${SOURCE_BASENAME}.npda.dot ${SOURCE_DIR}/${SOURCE_BASENAME}.npda.states)
    add_custom_target(${TARGET_NAME} DEPENDS ${OUTPUT_FILES})
    set_property(TARGET ${TARGET_NAME} PROPERTY EXCLUDE_FROM_ALL TRUE)
    add_custom_target(clean_${TARGET_NAME} COMMAND rm -f ${OUTPUT_FILES})

    if(DEFINED DOXYGEN_DOT_EXECUTABLE)
        add_custom_target(${TARGET_NAME}_npda_png DEPENDS ${SOURCE_DIR}/${SOURCE_BASENAME}.npda.png)
        add_custom_target(clean_${TARGET_NAME}_npda_png COMMAND rm -f ${SOURCE_DIR}/${SOURCE_BASENAME}.npda.png)

        add_custom_target(${TARGET_NAME}_dot_png DEPENDS ${TARGET_NAME}_npda_png)
        add_custom_target(clean_${TARGET_NAME}_dot_png DEPENDS clean_${TARGET_NAME}_npda_png)
    endif()
endfunction()
