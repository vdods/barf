###############################################################################
# reflex-development specific functions
###############################################################################

# This is similar to reflex_add_source, but runs against the reflex built in
# the project binary dir and the targets dir within this project source dir
# and adds some other useful targets.  Only to be used for barf development.
function(reflex_add_source_dev SOURCE_FILE TARGET_NAME)
    get_filename_component(SOURCE_FILE_EXT ${SOURCE_FILE} EXT)
    if(NOT (${SOURCE_FILE_EXT} STREQUAL ".reflex"))
        message(FATAL_ERROR "Source file \"${SOURCE_FILE}\" in call to reflex_add_source_dev does not have extension \".reflex\"")
    endif()
    get_filename_component(SOURCE_DIR ${SOURCE_FILE} DIRECTORY)
    get_filename_component(SOURCE_BASENAME ${SOURCE_FILE} NAME_WE)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/dev)

    __reflex_add_source__impl(${PROJECT_BINARY_DIR}/bin/reflex ${SOURCE_DIR} ${SOURCE_BASENAME} TRUE ${PROJECT_SOURCE_DIR}/targets ${PROJECT_BINARY_DIR}/dev force_${TARGET_NAME} reflex)

    set(OUTPUT_FILES
        ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.cpp
        ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.hpp
        ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.dfa.dot
        ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.nfa.dot
    )

    add_custom_target(${TARGET_NAME} DEPENDS ${OUTPUT_FILES})
    set_property(TARGET ${TARGET_NAME} PROPERTY EXCLUDE_FROM_ALL TRUE)
    add_custom_target(clean_${TARGET_NAME} COMMAND rm -f ${OUTPUT_FILES} ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.dfa.png ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.nfa.png)

    add_custom_target(
            diff_${TARGET_NAME}
        DEPENDS
            ${TARGET_NAME}
#             ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.cpp
#             ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.hpp
        COMMAND
            diff --report-identical-files ${SOURCE_DIR}/${SOURCE_BASENAME}.cpp ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.cpp
        COMMAND
            diff --report-identical-files ${SOURCE_DIR}/${SOURCE_BASENAME}.hpp ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.hpp
    )

    if(DEFINED DOXYGEN_DOT_EXECUTABLE)
        add_custom_target(${TARGET_NAME}_dfa_png DEPENDS ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.dfa.png)
        add_custom_target(clean_${TARGET_NAME}_dfa_png COMMAND rm -f ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.dfa.png)

        add_custom_target(${TARGET_NAME}_nfa_png DEPENDS ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.nfa.png)
        add_custom_target(clean_${TARGET_NAME}_nfa_png COMMAND rm -f ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.nfa.png)

        add_custom_target(${TARGET_NAME}_dot_png DEPENDS ${TARGET_NAME}_dfa_png ${TARGET_NAME}_nfa_png)
        add_custom_target(clean_${TARGET_NAME}_dot_png DEPENDS clean_${TARGET_NAME}_dfa_png clean_${TARGET_NAME}_nfa_png)
    endif()
endfunction()

# This is a bit silly, but it does the same thing as reflex_add_source_dev except it runs it using dev_reflex.
function(reflex_add_source_metadev SOURCE_FILE TARGET_NAME DEPENDENT_TARGET_NAME)
    get_filename_component(SOURCE_FILE_EXT ${SOURCE_FILE} EXT)
    if(NOT (${SOURCE_FILE_EXT} STREQUAL ".reflex"))
        message(FATAL_ERROR "Source file \"${SOURCE_FILE}\" in call to reflex_add_source_metadev does not have extension \".reflex\"")
    endif()
    get_filename_component(SOURCE_DIR ${SOURCE_FILE} DIRECTORY)
    get_filename_component(SOURCE_BASENAME ${SOURCE_FILE} NAME_WE)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/metadev)
    __reflex_add_source__impl(${PROJECT_BINARY_DIR}/bin/dev_reflex ${SOURCE_DIR} ${SOURCE_BASENAME} TRUE ${PROJECT_SOURCE_DIR}/targets ${PROJECT_BINARY_DIR}/metadev force_${TARGET_NAME} dev_reflex)

    set(OUTPUT_FILES
        ${PROJECT_BINARY_DIR}/metadev/${SOURCE_BASENAME}.cpp
        ${PROJECT_BINARY_DIR}/metadev/${SOURCE_BASENAME}.hpp
        ${PROJECT_BINARY_DIR}/metadev/${SOURCE_BASENAME}.dfa.dot
        ${PROJECT_BINARY_DIR}/metadev/${SOURCE_BASENAME}.nfa.dot
    )
    add_custom_target(${TARGET_NAME} DEPENDS ${OUTPUT_FILES})
    add_custom_target(clean_${TARGET_NAME} COMMAND rm -f ${OUTPUT_FILES} ${PROJECT_BINARY_DIR}/metadev/${SOURCE_BASENAME}.dfa.png ${PROJECT_BINARY_DIR}/metadev/${SOURCE_BASENAME}.nfa.png)

    add_custom_target(
            diff_${TARGET_NAME}
        DEPENDS
            ${TARGET_NAME}
            ${DEPENDENT_TARGET_NAME}
        COMMAND
            diff --report-identical-files ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.cpp ${PROJECT_BINARY_DIR}/metadev/${SOURCE_BASENAME}.cpp
        COMMAND
            diff --report-identical-files ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.hpp ${PROJECT_BINARY_DIR}/metadev/${SOURCE_BASENAME}.hpp
        COMMAND
            diff --report-identical-files ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.nfa.dot ${PROJECT_BINARY_DIR}/metadev/${SOURCE_BASENAME}.nfa.dot
        COMMAND
            diff --report-identical-files ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.dfa.dot ${PROJECT_BINARY_DIR}/metadev/${SOURCE_BASENAME}.dfa.dot
    )

    if(DEFINED DOXYGEN_DOT_EXECUTABLE)
        add_custom_target(${TARGET_NAME}_dfa_png DEPENDS ${PROJECT_BINARY_DIR}/metadev/${SOURCE_BASENAME}.dfa.png)
        add_custom_target(clean_${TARGET_NAME}_dfa_png COMMAND rm -f ${PROJECT_BINARY_DIR}/metadev/${SOURCE_BASENAME}.dfa.png)

        add_custom_target(${TARGET_NAME}_nfa_png DEPENDS ${PROJECT_BINARY_DIR}/metadev/${SOURCE_BASENAME}.nfa.png)
        add_custom_target(clean_${TARGET_NAME}_nfa_png COMMAND rm -f ${PROJECT_BINARY_DIR}/metadev/${SOURCE_BASENAME}.nfa.png)

        add_custom_target(${TARGET_NAME}_dot_png DEPENDS ${TARGET_NAME}_dfa_png ${TARGET_NAME}_nfa_png)
        add_custom_target(clean_${TARGET_NAME}_dot_png DEPENDS clean_${TARGET_NAME}_dfa_png clean_${TARGET_NAME}_nfa_png)
    endif()
endfunction()
