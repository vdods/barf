###############################################################################
# reflex-development specific functions
###############################################################################

function(reflex_add_source_dev SOURCE_FILE TARGET_NAME)
    get_filename_component(SOURCE_FILE_EXT ${SOURCE_FILE} EXT)
    if(NOT (${SOURCE_FILE_EXT} STREQUAL ".reflex"))
        message(FATAL_ERROR "Source file \"${SOURCE_FILE}\" in call to reflex_add_source_dev does not have extension \".reflex\"")
    endif()
    get_filename_component(SOURCE_DIR ${SOURCE_FILE} DIRECTORY)
    get_filename_component(SOURCE_BASENAME ${SOURCE_FILE} NAME_WE)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/dev)
    __reflex_add_source__impl(${PROJECT_BINARY_DIR}/bin/reflex ${SOURCE_DIR} ${SOURCE_BASENAME} TRUE ${PROJECT_SOURCE_DIR}/targets ${PROJECT_BINARY_DIR}/dev)

    set(OUTPUT_FILES ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.cpp ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.hpp ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.dfa.dot ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.nfa.dot)
    add_custom_target(${TARGET_NAME} DEPENDS ${OUTPUT_FILES})
    add_custom_target(clean_${TARGET_NAME} COMMAND rm -f ${OUTPUT_FILES} ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.dfa.png ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.nfa.png)

    add_custom_target(
            diff_${TARGET_NAME}
        DEPENDS
            ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.cpp
            ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.hpp
        COMMAND
            diff ${SOURCE_DIR}/${SOURCE_BASENAME}.cpp ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.cpp
        COMMAND
            diff ${SOURCE_DIR}/${SOURCE_BASENAME}.hpp ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.hpp
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
