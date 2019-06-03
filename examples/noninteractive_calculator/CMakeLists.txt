# Suite of tests for BARF

cmake_minimum_required(VERSION 3.10) # I had problems if I only specified "VERSION 3".
project(barftest)

# Set and require the C++11 standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# Helper target(s)

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

reflex_add_source(${barftest_SOURCE_DIR}/CalcSanner.reflex ${barftest_SOURCE_DIR} CalcScanner ${PROJECT_BINARY_DIR})
trison_add_source(${barftest_SOURCE_DIR}/CalcParser.trison ${barftest_SOURCE_DIR} CalcParser ${PROJECT_BINARY_DIR})

set(barftest_SOURCES
    ast.cpp
    ast.hpp
    CalcParser.cpp
    CalcParser.hpp
    CalcParser.trison
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
target_link_libraries(barftest PUBLIC Strict)
