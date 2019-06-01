###############################################################################
# CMake functions for compiling reflex sources into C++ as well as creating
# named targets for them.
###############################################################################

# Don't use this function directly.  Use reflex_add_source instead.
function(__reflex_add_source__impl REFLEX_BINARY SOURCE_DIR SOURCE_BASENAME SPECIFY_TARGETS_DIR TARGETS_DIR OUTPUT_DIR FORCE_TARGET_NAME BINARY_DEPENDENCY)
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
        ${BINARY_DEPENDENCY}
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

    # This target forces the build to happen -- command should be identical to the actual build rule above.
    add_custom_target(
        ${FORCE_TARGET_NAME}
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
    set_property(TARGET ${FORCE_TARGET_NAME} EXCLUDE_FROM_ALL TRUE)

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

# SOURCE_FILE is the .reflex file.  TARGET_NAME is the target that can be invoked to build this particular scanner.
function(reflex_add_source SOURCE_FILE TARGET_NAME)
    if(NOT DEFINED barf_REFLEX_BINARY)
        message(FATAL_ERROR "Must specify barf_REFLEX_BINARY")
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
    if(NOT (${SOURCE_FILE_EXT} STREQUAL ".reflex"))
        message(FATAL_ERROR "Source file \"${SOURCE_FILE}\" in call to reflex_add_source does not have extension \".reflex\"")
    endif()
    get_filename_component(SOURCE_DIR ${SOURCE_FILE} DIRECTORY)
    get_filename_component(SOURCE_BASENAME ${SOURCE_FILE} NAME_WE)
    __reflex_add_source__impl(${barf_REFLEX_BINARY} ${SOURCE_DIR} ${SOURCE_BASENAME} ${barf_ENABLE_TARGETS_DIR_OVERRIDE} "${barf_TARGETS_DIR_OVERRIDE}" ${SOURCE_DIR} force_${TARGET_NAME} ${barf_REFLEX_BINARY})

    set(OUTPUT_FILES ${SOURCE_DIR}/${SOURCE_BASENAME}.cpp ${SOURCE_DIR}/${SOURCE_BASENAME}.hpp ${SOURCE_DIR}/${SOURCE_BASENAME}.dfa.dot ${SOURCE_DIR}/${SOURCE_BASENAME}.nfa.dot)
    add_custom_target(${TARGET_NAME} DEPENDS ${OUTPUT_FILES})
    set_property(TARGET ${TARGET_NAME} EXCLUDE_FROM_ALL TRUE)
    add_custom_target(clean_${TARGET_NAME} COMMAND rm -f ${OUTPUT_FILES} ${SOURCE_DIR}/${SOURCE_BASENAME}.dfa.png ${SOURCE_DIR}/${SOURCE_BASENAME}.nfa.png)

    if(DEFINED DOXYGEN_DOT_EXECUTABLE)
        add_custom_target(${TARGET_NAME}_dfa_png DEPENDS ${SOURCE_DIR}/${SOURCE_BASENAME}.dfa.png)
        add_custom_target(clean_${TARGET_NAME}_dfa_png COMMAND rm -f ${SOURCE_DIR}/${SOURCE_BASENAME}.dfa.png)

        add_custom_target(${TARGET_NAME}_nfa_png DEPENDS ${SOURCE_DIR}/${SOURCE_BASENAME}.nfa.png)
        add_custom_target(clean_${TARGET_NAME}_nfa_png COMMAND rm -f ${SOURCE_DIR}/${SOURCE_BASENAME}.nfa.png)

        add_custom_target(${TARGET_NAME}_dot_png DEPENDS ${TARGET_NAME}_dfa_png ${TARGET_NAME}_nfa_png)
        add_custom_target(clean_${TARGET_NAME}_dot_png DEPENDS clean_${TARGET_NAME}_dfa_png clean_${TARGET_NAME}_nfa_png)
    endif()
endfunction()
