# example app which uses reflex and trison

cmake_minimum_required(VERSION 3.10) # I had problems if I only specified "VERSION 3".
project(calc)

# This defines rules for adding reflex and trison sources to produce C++ scanners and parsers.
# It requires having installed the barf package; the find_package function looks for the file
# barf-config.cmake in various locations to determine barf_DIR such that
# ${barf_DIR}/barf-config.cmake is a valid path, and then includes that cmake script.
find_package(barf REQUIRED)

# Options to correctly link the standard C++ lib on Mac.
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin") # This is the correct way to detect Mac OS X operating system -- see http://www.openguru.com/2009/04/cmake-detecting-platformoperating.html
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
    if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang") # GCC ("GNU") probably would require a different option
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif()
endif()

# Setup proper compiler flags for Debug and Release builds
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#     set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g2")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g2")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # TODO
endif()

# Helper target(s)

add_library(C++11 INTERFACE)
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_compile_options(C++11 INTERFACE -std=c++11)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # TODO
endif()

add_library(Strict INTERFACE)
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_compile_options(Strict INTERFACE -Wall -Werror)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # TODO
endif()

add_library(SaveTemps INTERFACE)
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_compile_options(SaveTemps INTERFACE -save-temps)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # TODO
endif()

# Executables

# set(BARF_TARGETS_DIR ${PROJECT_BINARY_DIR}/${BARF_TARGETS_PATH})
#
# add_custom_command(
#     OUTPUT ${calc_SOURCE_DIR}/Scanner.cpp ${calc_SOURCE_DIR}/Scanner.hpp
#     COMMAND ${REFLEX_BINARY} -I ${BARF_TARGETS_DIR} ${calc_SOURCE_DIR}/Scanner.reflex -o ${calc_SOURCE_DIR}
#     DEPENDS ${calc_SOURCE_DIR}/Scanner.reflex ${BARF_TARGETS_DIR}/reflex.cpp.header.codespec ${BARF_TARGETS_DIR}/reflex.cpp.implementation.codespec ${BARF_TARGETS_DIR}/reflex.cpp.targetspec)
#
# add_custom_command(
#     OUTPUT ${calc_SOURCE_DIR}/Parser.cpp ${calc_SOURCE_DIR}/Parser.hpp
#     COMMAND ${TRISON_BINARY} -I ${BARF_TARGETS_DIR} ${calc_SOURCE_DIR}/Parser.trison -o ${calc_SOURCE_DIR} -S Parser.states --generate-npda-dot-graph Parser.npda.dot
#     DEPENDS ${calc_SOURCE_DIR}/Parser.trison ${BARF_TARGETS_DIR}/trison.cpp.header.codespec ${BARF_TARGETS_DIR}/trison.cpp.implementation.codespec ${BARF_TARGETS_DIR}/trison.cpp.dpda.header.codespec ${BARF_TARGETS_DIR}/trison.cpp.dpda.implementation.codespec ${BARF_TARGETS_DIR}/trison.cpp.npda.header.codespec ${BARF_TARGETS_DIR}/trison.cpp.npda.implementation.codespec ${BARF_TARGETS_DIR}/trison.cpp.targetspec)

reflex_add_source(${calc_SOURCE_DIR}/Scanner.reflex Scanner)
trison_add_source(${calc_SOURCE_DIR}/Parser.trison Parser)

set(calc_SOURCES
    Parser.cpp
    Parser.hpp
    Parser.trison
    Scanner.cpp
    Scanner.hpp
    Scanner.reflex
)
add_executable(calc ${calc_SOURCES})
target_link_libraries(calc PUBLIC C++11 Strict)
