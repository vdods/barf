# Suite of tests for BARF

cmake_minimum_required(VERSION 3.0) # I had problems if I only specified "VERSION 3".
project(barftest)

find_path(REFLEX_BINARY "path to reflex binary" PATHS $ENV{REFLEX_BINARY} NO_DEFAULT_PATH)
find_path(TRISON_BINARY "path to trison binary" PATHS $ENV{TRISON_BINARY} NO_DEFAULT_PATH)
find_path(BARF_TARGETS_PATH "path to barf targets" PATHS $ENV{BARF_TARGETS_PATH} NO_DEFAULT_PATH)

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

set(BARF_TARGETS_DIR ${PROJECT_BINARY_DIR}/${BARF_TARGETS_PATH})

add_custom_command(
    OUTPUT ${barftest_SOURCE_DIR}/Scanner.cpp ${barftest_SOURCE_DIR}/Scanner.hpp
    COMMAND ${REFLEX_BINARY} -I ${BARF_TARGETS_DIR} ${barftest_SOURCE_DIR}/Scanner.reflex -o ${barftest_SOURCE_DIR}
    DEPENDS ${barftest_SOURCE_DIR}/Scanner.reflex ${BARF_TARGETS_DIR}/reflex.cpp.header.codespec ${BARF_TARGETS_DIR}/reflex.cpp.implementation.codespec ${BARF_TARGETS_DIR}/reflex.cpp.targetspec)

# add_custom_command(
#     OUTPUT ${barftest_SOURCE_DIR}/Parser.cpp ${barftest_SOURCE_DIR}/Parser.hpp
#     COMMAND ${TRISON_BINARY} -I ${BARF_TARGETS_DIR} ${barftest_SOURCE_DIR}/Parser.trison -o ${barftest_SOURCE_DIR} -S Parser.states --generate-npda-dot-graph Parser.npda.dot
#     DEPENDS ${barftest_SOURCE_DIR}/Parser.trison ${BARF_TARGETS_DIR}/trison.cpp.header.codespec ${BARF_TARGETS_DIR}/trison.cpp.implementation.codespec ${BARF_TARGETS_DIR}/trison.cpp.dpda.header.codespec ${BARF_TARGETS_DIR}/trison.cpp.dpda.implementation.codespec ${BARF_TARGETS_DIR}/trison.cpp.npda.header.codespec ${BARF_TARGETS_DIR}/trison.cpp.npda.implementation.codespec ${BARF_TARGETS_DIR}/trison.cpp.targetspec)

set(barftest_SOURCES
    ast.cpp
    ast.hpp
    log.cpp
    log.hpp
    main.cpp
    Scanner.cpp
    Scanner.hpp
    Scanner.reflex
    util.cpp
    util.hpp
)
add_executable(barftest ${barftest_SOURCES})
target_link_libraries(barftest PUBLIC C++11 Strict)
