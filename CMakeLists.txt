###############################################################################
# BARF (Bison Awesomely Replaced, Flex) -- a suite of sweet compiler tools.
###############################################################################

cmake_minimum_required(VERSION 3.10)
project(barf)

find_path(STABLE_REFLEX_BINARY "absolute path to stable reflex binary (only needed for barf developers)" PATHS $ENV{STABLE_REFLEX_BINARY} NO_DEFAULT_PATH)
find_path(STABLE_TRISON_BINARY "absolute path to stable trison binary (only needed for barf developers)" PATHS $ENV{STABLE_TRISON_BINARY} NO_DEFAULT_PATH)
find_path(STABLE_BARF_TARGETS_DIR "absolute path to stable targets directory (only needed for barf developers)" PATHS $ENV{STABLE_BARF_TARGETS_DIR} NO_DEFAULT_PATH)

# Options to correctly link the standard C++ lib on Mac.
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin") # This is the correct way to detect Mac OS X operating system -- see http://www.openguru.com/2009/04/cmake-detecting-platformoperating.html
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
    if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang") # GCC ("GNU") probably would require a different option
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif()
endif()

###############################################################################
# Dependencies
###############################################################################

find_package(Doxygen REQUIRED dot)
# TODO: Hook up doxygen config and generation

# Helper target(s)

# add_library(C++11 INTERFACE)
# if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#     target_compile_options(C++11 INTERFACE -std=c++11)
# endif()

add_library(Strict INTERFACE)
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_compile_options(Strict INTERFACE -Wall -Werror)
endif()

add_library(SaveTemps INTERFACE)
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_compile_options(SaveTemps INTERFACE -save-temps)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # TODO
endif()

###############################################################################
# Misc
###############################################################################

set(STABLE_REFLEX_TARGET_DEPENDENCIES
    ${STABLE_BARF_TARGETS_DIR}/reflex.cpp.header.codespec
    ${STABLE_BARF_TARGETS_DIR}/reflex.cpp.implementation.codespec
    ${STABLE_BARF_TARGETS_DIR}/reflex.cpp.targetspec
)

set(STABLE_TRISON_TARGET_DEPENDENCIES
    ${STABLE_BARF_TARGETS_DIR}/trison.cpp.header.codespec
    ${STABLE_BARF_TARGETS_DIR}/trison.cpp.implementation.codespec
    ${STABLE_BARF_TARGETS_DIR}/trison.cpp.npda.grammar.header.codespec
    ${STABLE_BARF_TARGETS_DIR}/trison.cpp.npda.grammar.implementation.codespec
    ${STABLE_BARF_TARGETS_DIR}/trison.cpp.npda.header.codespec
    ${STABLE_BARF_TARGETS_DIR}/trison.cpp.npda.implementation.codespec
    ${STABLE_BARF_TARGETS_DIR}/trison.cpp.npda.npda.header.codespec
    ${STABLE_BARF_TARGETS_DIR}/trison.cpp.npda.npda.implementation.codespec
    ${STABLE_BARF_TARGETS_DIR}/trison.cpp.targetspec
)

###############################################################################
# Libraries
###############################################################################

#
# barf_commonlang_and_targetspec
#

add_custom_command(
    OUTPUT
        ${PROJECT_SOURCE_DIR}/lib/commonlang/barf_commonlang_scanner.cpp
        ${PROJECT_SOURCE_DIR}/lib/commonlang/barf_commonlang_scanner.hpp
    BYPRODUCTS
        ${PROJECT_SOURCE_DIR}/lib/commonlang/barf_commonlang_scanner.nfa.dot
        ${PROJECT_SOURCE_DIR}/lib/commonlang/barf_commonlang_scanner.dfa.dot
    COMMAND
        ${STABLE_REFLEX_BINARY}
        -I ${STABLE_BARF_TARGETS_DIR}
        ${PROJECT_SOURCE_DIR}/lib/commonlang/barf_commonlang_scanner.reflex
        -o ${PROJECT_SOURCE_DIR}/lib/commonlang
        --generate-nfa-dot-graph barf_commonlang_scanner.nfa.dot
        --generate-dfa-dot-graph barf_commonlang_scanner.dfa.dot
    MAIN_DEPENDENCY
        ${PROJECT_SOURCE_DIR}/lib/commonlang/barf_commonlang_scanner.reflex
    DEPENDS
        ${STABLE_REFLEX_TARGET_DEPENDENCIES}
)

add_custom_command(
    OUTPUT
        ${PROJECT_SOURCE_DIR}/lib/targetspec/barf_targetspec_parser.cpp
        ${PROJECT_SOURCE_DIR}/lib/targetspec/barf_targetspec_parser.hpp
    BYPRODUCTS
        ${PROJECT_SOURCE_DIR}/lib/targetspec/barf_targetspec_parser.npda.dot
        ${PROJECT_SOURCE_DIR}/lib/targetspec/barf_targetspec_parser.npda.states
    COMMAND
        ${STABLE_TRISON_BINARY}
        -I ${STABLE_BARF_TARGETS_DIR}
        ${PROJECT_SOURCE_DIR}/lib/targetspec/barf_targetspec_parser.trison
        -o ${PROJECT_SOURCE_DIR}/lib/targetspec
        --generate-npda-dot-graph barf_targetspec_parser.npda.dot
        --generate-npda-states-file barf_targetspec_parser.npda.states
    MAIN_DEPENDENCY
        ${PROJECT_SOURCE_DIR}/lib/targetspec/barf_targetspec_parser.trison
    DEPENDS
        ${STABLE_TRISON_TARGET_DEPENDENCIES}
)

set(barf_commonlang_and_targetspec_SOURCES
    lib/commonlang/barf_commonlang.hpp
    lib/commonlang/barf_commonlang_ast.cpp
    lib/commonlang/barf_commonlang_ast.hpp
    lib/commonlang/barf_commonlang_scanner.cpp
    lib/commonlang/barf_commonlang_scanner.hpp
    lib/commonlang/barf_commonlang_scanner.reflex
    lib/targetspec/barf_targetspec.hpp
    lib/targetspec/barf_targetspec_ast.cpp
    lib/targetspec/barf_targetspec_ast.hpp
    lib/targetspec/barf_targetspec_parser.cpp
    lib/targetspec/barf_targetspec_parser.hpp
    lib/targetspec/barf_targetspec_parser.trison
)

add_library(barf_commonlang_and_targetspec ${barf_commonlang_and_targetspec_SOURCES})
target_include_directories(barf_commonlang_and_targetspec PUBLIC ${PROJECT_SOURCE_DIR}/lib/commonlang)
target_include_directories(barf_commonlang_and_targetspec PUBLIC ${PROJECT_SOURCE_DIR}/lib/targetspec)
target_link_libraries(barf_commonlang_and_targetspec PUBLIC Strict barf_core barf_preprocessor)

#
# barf_core
#

set(barf_core_SOURCES
    lib/core/barf.cpp
    lib/core/barf.hpp
    lib/core/barf_ast.cpp
    lib/core/barf_ast.hpp
    lib/core/barf_commandlineparser.cpp
    lib/core/barf_commandlineparser.hpp
    lib/core/barf_compiletimeasserts.cpp
    lib/core/barf_compiletimeasserts.hpp
    lib/core/barf_enums.cpp
    lib/core/barf_enums.hpp
    lib/core/barf_filoc.cpp
    lib/core/barf_filoc.hpp
    lib/core/barf_graph.cpp
    lib/core/barf_graph.hpp
    lib/core/barf_inputbase.cpp
    lib/core/barf_inputbase.hpp
    lib/core/barf_list.hpp
    lib/core/barf_message.cpp
    lib/core/barf_message.hpp
    lib/core/barf_optionsbase.cpp
    lib/core/barf_optionsbase.hpp
    lib/core/barf_pointer.hpp
    lib/core/barf_searchpath.cpp
    lib/core/barf_searchpath.hpp
    lib/core/barf_types.hpp
    lib/core/barf_util.cpp
    lib/core/barf_util.hpp
    lib/core/barf_weakreference.hpp
)

add_library(barf_core ${barf_core_SOURCES})
target_include_directories(barf_core PUBLIC ${PROJECT_SOURCE_DIR}/lib/core)
target_link_libraries(barf_core PUBLIC Strict)

#
# barf_preprocessor
#

add_custom_command(
    OUTPUT
        ${PROJECT_SOURCE_DIR}/lib/preprocessor/barf_preprocessor_scanner.cpp
        ${PROJECT_SOURCE_DIR}/lib/preprocessor/barf_preprocessor_scanner.hpp
    BYPRODUCTS
        ${PROJECT_SOURCE_DIR}/lib/preprocessor/barf_preprocessor_scanner.nfa.dot
        ${PROJECT_SOURCE_DIR}/lib/preprocessor/barf_preprocessor_scanner.dfa.dot
    COMMAND
        ${STABLE_REFLEX_BINARY}
        -I ${STABLE_BARF_TARGETS_DIR}
        ${PROJECT_SOURCE_DIR}/lib/preprocessor/barf_preprocessor_scanner.reflex
        -o ${PROJECT_SOURCE_DIR}/lib/preprocessor
        --generate-nfa-dot-graph barf_preprocessor_scanner.nfa.dot
        --generate-dfa-dot-graph barf_preprocessor_scanner.dfa.dot
    MAIN_DEPENDENCY
        ${PROJECT_SOURCE_DIR}/lib/preprocessor/barf_preprocessor_scanner.reflex
    DEPENDS
        ${STABLE_REFLEX_TARGET_DEPENDENCIES}
)

add_custom_command(
    OUTPUT
        ${PROJECT_SOURCE_DIR}/lib/preprocessor/barf_preprocessor_parser.cpp
        ${PROJECT_SOURCE_DIR}/lib/preprocessor/barf_preprocessor_parser.hpp
    BYPRODUCTS
        ${PROJECT_SOURCE_DIR}/lib/preprocessor/barf_preprocessor_parser.npda.dot
        ${PROJECT_SOURCE_DIR}/lib/preprocessor/barf_preprocessor_parser.npda.states
    COMMAND
        ${STABLE_TRISON_BINARY}
        -I ${STABLE_BARF_TARGETS_DIR}
        ${PROJECT_SOURCE_DIR}/lib/preprocessor/barf_preprocessor_parser.trison
        -o ${PROJECT_SOURCE_DIR}/lib/preprocessor
        --generate-npda-dot-graph barf_preprocessor_parser.npda.dot
        --generate-npda-states-file barf_preprocessor_parser.npda.states
    MAIN_DEPENDENCY
        ${PROJECT_SOURCE_DIR}/lib/preprocessor/barf_preprocessor_parser.trison
    DEPENDS
        ${STABLE_TRISON_TARGET_DEPENDENCIES}
)

set(barf_preprocessor_SOURCES
    lib/preprocessor/barf_preprocessor.hpp
    lib/preprocessor/barf_preprocessor_ast.cpp
    lib/preprocessor/barf_preprocessor_ast_execute.cpp
    lib/preprocessor/barf_preprocessor_ast.hpp
    lib/preprocessor/barf_preprocessor_ast_print.cpp
    lib/preprocessor/barf_preprocessor_parser.cpp
    lib/preprocessor/barf_preprocessor_parser.hpp
    lib/preprocessor/barf_preprocessor_parser.trison
    lib/preprocessor/barf_preprocessor_scanner.cpp
    lib/preprocessor/barf_preprocessor_scanner.hpp
    lib/preprocessor/barf_preprocessor_scanner.reflex
    lib/preprocessor/barf_preprocessor_symboltable.cpp
    lib/preprocessor/barf_preprocessor_symboltable.hpp
    lib/preprocessor/barf_preprocessor_textifier.cpp
    lib/preprocessor/barf_preprocessor_textifier.hpp
)

add_library(barf_preprocessor ${barf_preprocessor_SOURCES})
target_include_directories(barf_preprocessor PUBLIC ${PROJECT_SOURCE_DIR}/lib/preprocessor)
target_link_libraries(barf_preprocessor PUBLIC Strict barf_core)

#
# barf_regex
#

add_custom_command(
    OUTPUT
        ${PROJECT_SOURCE_DIR}/lib/regex/barf_regex_parser.cpp
        ${PROJECT_SOURCE_DIR}/lib/regex/barf_regex_parser.hpp
    BYPRODUCTS
        ${PROJECT_SOURCE_DIR}/lib/regex/barf_regex_parser.npda.dot
        ${PROJECT_SOURCE_DIR}/lib/regex/barf_regex_parser.npda.states
    COMMAND
        ${STABLE_TRISON_BINARY}
        -I ${STABLE_BARF_TARGETS_DIR}
        ${PROJECT_SOURCE_DIR}/lib/regex/barf_regex_parser.trison
        -o ${PROJECT_SOURCE_DIR}/lib/regex
        --generate-npda-dot-graph barf_regex_parser.npda.dot
        --generate-npda-states-file barf_regex_parser.npda.states
    MAIN_DEPENDENCY
        ${PROJECT_SOURCE_DIR}/lib/regex/barf_regex_parser.trison
    DEPENDS
        ${STABLE_TRISON_TARGET_DEPENDENCIES}
)

set(barf_regex_SOURCES
    lib/regex/barf_regex.cpp
    lib/regex/barf_regex.hpp
    lib/regex/barf_regex_ast.cpp
    lib/regex/barf_regex_ast.hpp
    lib/regex/barf_regex_dfa.cpp
    lib/regex/barf_regex_dfa.hpp
    lib/regex/barf_regex_graph.cpp
    lib/regex/barf_regex_graph.hpp
    lib/regex/barf_regex_nfa.cpp
    lib/regex/barf_regex_nfa.hpp
    lib/regex/barf_regex_parser.cpp
    lib/regex/barf_regex_parser.hpp
    lib/regex/barf_regex_parser.trison
)

add_library(barf_regex ${barf_regex_SOURCES})
target_include_directories(barf_regex PUBLIC ${PROJECT_SOURCE_DIR}/lib/regex)
target_link_libraries(barf_regex PUBLIC Strict barf_core)

###############################################################################
# Executables
###############################################################################

#
# bpp (BARF Pre-Processor)
#

set(BPP_SOURCES
    app/bpp/bpp.hpp
    app/bpp/bpp_main.cpp
    app/bpp/bpp_options.cpp
    app/bpp/bpp_options.hpp
)
add_executable(bpp ${BPP_SOURCES})
target_include_directories(bpp PUBLIC ${PROJECT_SOURCE_DIR}/app/bpp)
target_link_libraries(bpp PUBLIC Strict barf_core barf_preprocessor)

#
# reflex (scanner generator)
#

add_custom_command(
    OUTPUT
        ${PROJECT_SOURCE_DIR}/app/reflex/reflex_parser.cpp
        ${PROJECT_SOURCE_DIR}/app/reflex/reflex_parser.hpp
    BYPRODUCTS
        ${PROJECT_SOURCE_DIR}/app/reflex/reflex_parser.npda.dot
        ${PROJECT_SOURCE_DIR}/app/reflex/reflex_parser.npda.states
    COMMAND
        ${STABLE_TRISON_BINARY}
        -I ${STABLE_BARF_TARGETS_DIR}
        ${PROJECT_SOURCE_DIR}/app/reflex/reflex_parser.trison
        -o ${PROJECT_SOURCE_DIR}/app/reflex
        --generate-npda-dot-graph reflex_parser.npda.dot
        --generate-npda-states-file reflex_parser.npda.states
    MAIN_DEPENDENCY
        ${PROJECT_SOURCE_DIR}/app/reflex/reflex_parser.trison
    DEPENDS
        ${STABLE_TRISON_TARGET_DEPENDENCIES}
)

set(REFLEX_SOURCES
    app/reflex/reflex.hpp
    app/reflex/reflex_ast.cpp
    app/reflex/reflex_ast.hpp
    app/reflex/reflex_automaton.cpp
    app/reflex/reflex_automaton.hpp
    app/reflex/reflex_codespecsymbols.cpp
    app/reflex/reflex_codespecsymbols.hpp
    app/reflex/reflex_main.cpp
    app/reflex/reflex_options.cpp
    app/reflex/reflex_options.hpp
    app/reflex/reflex_parser.cpp
    app/reflex/reflex_parser.hpp
    app/reflex/reflex_parser.trison
)
add_executable(reflex ${REFLEX_SOURCES})
target_include_directories(reflex PUBLIC ${PROJECT_SOURCE_DIR}/app/reflex)
target_link_libraries(reflex PUBLIC Strict barf_commonlang_and_targetspec barf_core barf_preprocessor barf_regex)

#
# trison (parser generator)
#

add_custom_command(
    OUTPUT
        ${PROJECT_SOURCE_DIR}/app/trison/trison_parser.cpp
        ${PROJECT_SOURCE_DIR}/app/trison/trison_parser.hpp
    BYPRODUCTS
        ${PROJECT_SOURCE_DIR}/app/trison/trison_parser.npda.dot
        ${PROJECT_SOURCE_DIR}/app/trison/trison_parser.npda.states
    COMMAND
        ${STABLE_TRISON_BINARY}
        -I ${STABLE_BARF_TARGETS_DIR}
        ${PROJECT_SOURCE_DIR}/app/trison/trison_parser.trison
        -o ${PROJECT_SOURCE_DIR}/app/trison
        --generate-npda-dot-graph trison_parser.npda.dot
        --generate-npda-states-file trison_parser.npda.states
    MAIN_DEPENDENCY
        ${PROJECT_SOURCE_DIR}/app/trison/trison_parser.trison
    DEPENDS
        ${STABLE_TRISON_TARGET_DEPENDENCIES}
)

set(TRISON_SOURCES
    app/trison/trison.hpp
    app/trison/trison_ast.cpp
    app/trison/trison_ast.hpp
    app/trison/trison_codespecsymbols.cpp
    app/trison/trison_codespecsymbols.hpp
    app/trison/trison_dpda.cpp
    app/trison/trison_dpda.hpp
    app/trison/trison_enums.cpp
    app/trison/trison_enums.hpp
    app/trison/trison_graph.cpp
    app/trison/trison_graph.hpp
    app/trison/trison_main.cpp
    app/trison/trison_npda.cpp
    app/trison/trison_npda.hpp
    app/trison/trison_options.cpp
    app/trison/trison_options.hpp
    app/trison/trison_parser.cpp
    app/trison/trison_parser.hpp
    app/trison/trison_parser.trison
)
add_executable(trison ${TRISON_SOURCES})
target_include_directories(trison PUBLIC ${PROJECT_SOURCE_DIR}/app/trison)
target_link_libraries(trison PUBLIC Strict barf_commonlang_and_targetspec barf_core barf_preprocessor)

###############################################################################
# Testing
###############################################################################

# TODO
