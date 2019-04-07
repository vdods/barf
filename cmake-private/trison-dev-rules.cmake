###############################################################################
# trison-development specific functions
###############################################################################

function(trison_add_source_dev SOURCE_FILE TARGET_NAME)
    get_filename_component(SOURCE_FILE_EXT ${SOURCE_FILE} EXT)
    if(NOT (${SOURCE_FILE_EXT} STREQUAL ".trison"))
        message(FATAL_ERROR "Source file \"${SOURCE_FILE}\" in call to trison_add_source_dev does not have extension \".trison\"")
    endif()
    get_filename_component(SOURCE_DIR ${SOURCE_FILE} DIRECTORY)
    get_filename_component(SOURCE_BASENAME ${SOURCE_FILE} NAME_WE)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/dev)
    __trison_add_source__impl(${PROJECT_BINARY_DIR}/bin/trison ${SOURCE_DIR} ${SOURCE_BASENAME} TRUE ${PROJECT_SOURCE_DIR}/targets ${PROJECT_BINARY_DIR}/dev)

    set(OUTPUT_FILES ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.cpp ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.hpp ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.npda.dot ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.npda.states)
    add_custom_target(${TARGET_NAME} DEPENDS ${OUTPUT_FILES})
    add_custom_target(clean_${TARGET_NAME} COMMAND rm -f ${OUTPUT_FILES})

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
        add_custom_target(${TARGET_NAME}_npda_png DEPENDS ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.npda.png)
        add_custom_target(clean_${TARGET_NAME}_npda_png COMMAND rm -f ${PROJECT_BINARY_DIR}/dev/${SOURCE_BASENAME}.npda.png)

        add_custom_target(${TARGET_NAME}_dot_png DEPENDS ${TARGET_NAME}_npda_png)
        add_custom_target(clean_${TARGET_NAME}_dot_png DEPENDS clean_${TARGET_NAME}_npda_png)
    endif()
endfunction()
