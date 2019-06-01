###############################################################################
# trison-development specific functions
###############################################################################

# This is similar to reflex_add_source, but runs against the reflex built in
# the project binary dir and the targets dir within this project source dir
# and adds some other useful targets.  Only to be used for barf development.
function(trison_add_source_dev SOURCE_FILE OUTPUT_DIR TARGET_NAME)
    get_filename_component(SOURCE_FILE_EXT ${SOURCE_FILE} EXT)
    if(NOT (${SOURCE_FILE_EXT} STREQUAL ".trison"))
        message(FATAL_ERROR "Source file \"${SOURCE_FILE}\" in call to trison_add_source_dev does not have extension \".trison\"")
    endif()
    get_filename_component(SOURCE_DIR ${SOURCE_FILE} DIRECTORY)
    get_filename_component(SOURCE_BASENAME ${SOURCE_FILE} NAME_WE)

    file(MAKE_DIRECTORY ${OUTPUT_DIR})

    __trison_add_source__impl(${PROJECT_BINARY_DIR}/bin/trison ${SOURCE_DIR} ${SOURCE_BASENAME} TRUE ${PROJECT_SOURCE_DIR}/targets ${OUTPUT_DIR} force_${TARGET_NAME} trison)

    set(OUTPUT_FILES
        ${OUTPUT_DIR}/${SOURCE_BASENAME}.cpp
        ${OUTPUT_DIR}/${SOURCE_BASENAME}.hpp
        ${OUTPUT_DIR}/${SOURCE_BASENAME}.npda.dot
        ${OUTPUT_DIR}/${SOURCE_BASENAME}.npda.states
    )

    add_custom_target(${TARGET_NAME} DEPENDS ${OUTPUT_FILES})
    set_property(TARGET ${TARGET_NAME} PROPERTY EXCLUDE_FROM_ALL TRUE)
    add_custom_target(clean_${TARGET_NAME} COMMAND rm -f ${OUTPUT_FILES})

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
        add_custom_target(${TARGET_NAME}_npda_png DEPENDS ${OUTPUT_DIR}/${SOURCE_BASENAME}.npda.png)
        add_custom_target(clean_${TARGET_NAME}_npda_png COMMAND rm -f ${OUTPUT_DIR}/${SOURCE_BASENAME}.npda.png)

        add_custom_target(${TARGET_NAME}_dot_png DEPENDS ${TARGET_NAME}_npda_png)
        add_custom_target(clean_${TARGET_NAME}_dot_png DEPENDS clean_${TARGET_NAME}_npda_png)
    endif()
endfunction()

# This is a bit silly, but it does the same thing as trison_add_source_dev except it runs it using dev_trison.
function(trison_add_source_metadev SOURCE_FILE OUTPUT_DIR TARGET_NAME DEPENDENT_TARGET_NAME)
    get_filename_component(SOURCE_FILE_EXT ${SOURCE_FILE} EXT)
    if(NOT (${SOURCE_FILE_EXT} STREQUAL ".trison"))
        message(FATAL_ERROR "Source file \"${SOURCE_FILE}\" in call to trison_add_source_metadev does not have extension \".trison\"")
    endif()
    get_filename_component(SOURCE_DIR ${SOURCE_FILE} DIRECTORY)
    get_filename_component(SOURCE_BASENAME ${SOURCE_FILE} NAME_WE)

    file(RELATIVE_PATH RELATIVE_SOURCE_DIR ${PROJECT_SOURCE_DIR} ${SOURCE_DIR})
    set(DEV_COMPARISON_DIR ${PROJECT_BINARY_DIR}/dev/${RELATIVE_SOURCE_DIR}/generated)
    file(MAKE_DIRECTORY ${OUTPUT_DIR})

    __trison_add_source__impl(${PROJECT_BINARY_DIR}/bin/dev_trison ${SOURCE_DIR} ${SOURCE_BASENAME} TRUE ${PROJECT_SOURCE_DIR}/targets ${OUTPUT_DIR} force_${TARGET_NAME} dev_trison)

    set(OUTPUT_FILES
        ${OUTPUT_DIR}/${SOURCE_BASENAME}.cpp
        ${OUTPUT_DIR}/${SOURCE_BASENAME}.hpp
        ${OUTPUT_DIR}/${SOURCE_BASENAME}.npda.dot
        ${OUTPUT_DIR}/${SOURCE_BASENAME}.npda.states
    )
    add_custom_target(${TARGET_NAME} DEPENDS ${OUTPUT_FILES})
    add_custom_target(clean_${TARGET_NAME} COMMAND rm -f ${OUTPUT_FILES})

    add_custom_target(
            diff_${TARGET_NAME}
        DEPENDS
            ${TARGET_NAME}
            ${DEPENDENT_TARGET_NAME}
#             ${DEV_COMPARISON_DIR}/${SOURCE_BASENAME}.cpp
#             ${DEV_COMPARISON_DIR}/${SOURCE_BASENAME}.hpp
#             ${DEV_COMPARISON_DIR}/${SOURCE_BASENAME}.npda.dot
#             ${DEV_COMPARISON_DIR}/${SOURCE_BASENAME}.npda.states
#             ${OUTPUT_DIR}/${SOURCE_BASENAME}.cpp
#             ${OUTPUT_DIR}/${SOURCE_BASENAME}.hpp
#             ${OUTPUT_DIR}/${SOURCE_BASENAME}.npda.dot
#             ${OUTPUT_DIR}/${SOURCE_BASENAME}.npda.states
        COMMAND
            diff --report-identical-files ${DEV_COMPARISON_DIR}/${SOURCE_BASENAME}.cpp ${OUTPUT_DIR}/${SOURCE_BASENAME}.cpp
        COMMAND
            diff --report-identical-files ${DEV_COMPARISON_DIR}/${SOURCE_BASENAME}.hpp ${OUTPUT_DIR}/${SOURCE_BASENAME}.hpp
        COMMAND
            diff --report-identical-files ${DEV_COMPARISON_DIR}/${SOURCE_BASENAME}.npda.dot ${OUTPUT_DIR}/${SOURCE_BASENAME}.npda.dot
        COMMAND
            diff --report-identical-files ${DEV_COMPARISON_DIR}/${SOURCE_BASENAME}.npda.states ${OUTPUT_DIR}/${SOURCE_BASENAME}.npda.states
    )

    if(DEFINED DOXYGEN_DOT_EXECUTABLE)
        add_custom_target(${TARGET_NAME}_npda_png DEPENDS ${OUTPUT_DIR}/${SOURCE_BASENAME}.npda.png)
        add_custom_target(clean_${TARGET_NAME}_npda_png COMMAND rm -f ${OUTPUT_DIR}/${SOURCE_BASENAME}.npda.png)

        add_custom_target(${TARGET_NAME}_dot_png DEPENDS ${TARGET_NAME}_npda_png)
        add_custom_target(clean_${TARGET_NAME}_dot_png DEPENDS clean_${TARGET_NAME}_npda_png)
    endif()
endfunction()
