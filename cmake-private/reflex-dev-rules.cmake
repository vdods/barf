###############################################################################
# reflex-development specific functions
###############################################################################

# This is similar to reflex_add_source, but runs against the reflex built in
# the project binary dir and the targets dir within this project source dir
# and adds some other useful targets.  Only to be used for barf development.
function(reflex_add_source_dev SOURCE_FILE OUTPUT_DIR TARGET_NAME)
    get_filename_component(SOURCE_FILE_EXT ${SOURCE_FILE} EXT)
    if(NOT (${SOURCE_FILE_EXT} STREQUAL ".reflex"))
        message(FATAL_ERROR "Source file \"${SOURCE_FILE}\" in call to reflex_add_source_dev does not have extension \".reflex\"")
    endif()
    get_filename_component(SOURCE_DIR ${SOURCE_FILE} DIRECTORY)
    get_filename_component(SOURCE_BASENAME ${SOURCE_FILE} NAME_WE)

    file(MAKE_DIRECTORY ${OUTPUT_DIR})

    # FALSE for WITH_LINE_DIRECTIVES, so we don't get different paths between dev and metadev
    __reflex_add_source__impl(${PROJECT_BINARY_DIR}/bin/reflex ${SOURCE_DIR} ${SOURCE_BASENAME} FALSE ${PROJECT_BINARY_DIR} TRUE ${PROJECT_SOURCE_DIR}/targets ${OUTPUT_DIR} force_${TARGET_NAME} reflex)

    set(OUTPUT_FILES
        ${OUTPUT_DIR}/${SOURCE_BASENAME}.cpp
        ${OUTPUT_DIR}/${SOURCE_BASENAME}.hpp
        ${OUTPUT_DIR}/${SOURCE_BASENAME}.dfa.dot
        ${OUTPUT_DIR}/${SOURCE_BASENAME}.nfa.dot
    )

    add_custom_target(${TARGET_NAME} DEPENDS ${OUTPUT_FILES})
    set_property(TARGET ${TARGET_NAME} PROPERTY EXCLUDE_FROM_ALL TRUE)
    add_custom_target(clean_${TARGET_NAME} COMMAND rm -f ${OUTPUT_FILES} ${OUTPUT_DIR}/${SOURCE_BASENAME}.dfa.png ${OUTPUT_DIR}/${SOURCE_BASENAME}.nfa.png)

    add_custom_target(
            diff_${TARGET_NAME}
        DEPENDS
            ${TARGET_NAME}
#             ${OUTPUT_DIR}/${SOURCE_BASENAME}.cpp
#             ${OUTPUT_DIR}/${SOURCE_BASENAME}.hpp
        COMMAND
            diff --report-identical-files ${SOURCE_DIR}/${SOURCE_BASENAME}.cpp ${OUTPUT_DIR}/${SOURCE_BASENAME}.cpp
        COMMAND
            diff --report-identical-files ${SOURCE_DIR}/${SOURCE_BASENAME}.hpp ${OUTPUT_DIR}/${SOURCE_BASENAME}.hpp
    )

    if(DEFINED DOXYGEN_DOT_EXECUTABLE)
        add_custom_target(${TARGET_NAME}_dfa_png DEPENDS ${OUTPUT_DIR}/${SOURCE_BASENAME}.dfa.png)
        add_custom_target(clean_${TARGET_NAME}_dfa_png COMMAND rm -f ${OUTPUT_DIR}/${SOURCE_BASENAME}.dfa.png)

        add_custom_target(${TARGET_NAME}_nfa_png DEPENDS ${OUTPUT_DIR}/${SOURCE_BASENAME}.nfa.png)
        add_custom_target(clean_${TARGET_NAME}_nfa_png COMMAND rm -f ${OUTPUT_DIR}/${SOURCE_BASENAME}.nfa.png)

        add_custom_target(${TARGET_NAME}_dot_png DEPENDS ${TARGET_NAME}_dfa_png ${TARGET_NAME}_nfa_png)
        add_custom_target(clean_${TARGET_NAME}_dot_png DEPENDS clean_${TARGET_NAME}_dfa_png clean_${TARGET_NAME}_nfa_png)
    endif()
endfunction()

# This is a bit silly, but it does the same thing as reflex_add_source_dev except it runs it using dev_reflex.
function(reflex_add_source_metadev SOURCE_FILE OUTPUT_DIR TARGET_NAME DEPENDENT_TARGET_NAME)
    get_filename_component(SOURCE_FILE_EXT ${SOURCE_FILE} EXT)
    if(NOT (${SOURCE_FILE_EXT} STREQUAL ".reflex"))
        message(FATAL_ERROR "Source file \"${SOURCE_FILE}\" in call to reflex_add_source_metadev does not have extension \".reflex\"")
    endif()
    get_filename_component(SOURCE_DIR ${SOURCE_FILE} DIRECTORY)
    get_filename_component(SOURCE_BASENAME ${SOURCE_FILE} NAME_WE)

    file(RELATIVE_PATH RELATIVE_SOURCE_DIR ${PROJECT_SOURCE_DIR} ${SOURCE_DIR})
    set(DEV_COMPARISON_DIR ${PROJECT_BINARY_DIR}/dev/${RELATIVE_SOURCE_DIR}/generated)
    file(MAKE_DIRECTORY ${OUTPUT_DIR})

    # FALSE for WITH_LINE_DIRECTIVES, so we don't get different paths between dev and metadev
    __reflex_add_source__impl(${PROJECT_BINARY_DIR}/bin/dev_reflex ${SOURCE_DIR} ${SOURCE_BASENAME} FALSE ${PROJECT_BINARY_DIR} TRUE ${PROJECT_SOURCE_DIR}/targets ${OUTPUT_DIR} force_${TARGET_NAME} dev_reflex)

    set(OUTPUT_FILES
        ${OUTPUT_DIR}/${SOURCE_BASENAME}.cpp
        ${OUTPUT_DIR}/${SOURCE_BASENAME}.hpp
        ${OUTPUT_DIR}/${SOURCE_BASENAME}.dfa.dot
        ${OUTPUT_DIR}/${SOURCE_BASENAME}.nfa.dot
    )
    add_custom_target(${TARGET_NAME} DEPENDS ${OUTPUT_FILES})
    add_custom_target(clean_${TARGET_NAME} COMMAND rm -f ${OUTPUT_FILES} ${OUTPUT_DIR}/${SOURCE_BASENAME}.dfa.png ${OUTPUT_DIR}/${SOURCE_BASENAME}.nfa.png)

    add_custom_target(
            diff_${TARGET_NAME}
        DEPENDS
            ${TARGET_NAME}
            ${DEPENDENT_TARGET_NAME}
        COMMAND
            diff --report-identical-files ${DEV_COMPARISON_DIR}/${SOURCE_BASENAME}.cpp ${OUTPUT_DIR}/${SOURCE_BASENAME}.cpp
        COMMAND
            diff --report-identical-files ${DEV_COMPARISON_DIR}/${SOURCE_BASENAME}.hpp ${OUTPUT_DIR}/${SOURCE_BASENAME}.hpp
        COMMAND
            diff --report-identical-files ${DEV_COMPARISON_DIR}/${SOURCE_BASENAME}.nfa.dot ${OUTPUT_DIR}/${SOURCE_BASENAME}.nfa.dot
        COMMAND
            diff --report-identical-files ${DEV_COMPARISON_DIR}/${SOURCE_BASENAME}.dfa.dot ${OUTPUT_DIR}/${SOURCE_BASENAME}.dfa.dot
    )

    if(DEFINED DOXYGEN_DOT_EXECUTABLE)
        add_custom_target(${TARGET_NAME}_dfa_png DEPENDS ${OUTPUT_DIR}/${SOURCE_BASENAME}.dfa.png)
        add_custom_target(clean_${TARGET_NAME}_dfa_png COMMAND rm -f ${OUTPUT_DIR}/${SOURCE_BASENAME}.dfa.png)

        add_custom_target(${TARGET_NAME}_nfa_png DEPENDS ${OUTPUT_DIR}/${SOURCE_BASENAME}.nfa.png)
        add_custom_target(clean_${TARGET_NAME}_nfa_png COMMAND rm -f ${OUTPUT_DIR}/${SOURCE_BASENAME}.nfa.png)

        add_custom_target(${TARGET_NAME}_dot_png DEPENDS ${TARGET_NAME}_dfa_png ${TARGET_NAME}_nfa_png)
        add_custom_target(clean_${TARGET_NAME}_dot_png DEPENDS clean_${TARGET_NAME}_dfa_png clean_${TARGET_NAME}_nfa_png)
    endif()
endfunction()
