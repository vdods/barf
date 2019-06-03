# Suite of tests for BARF

cmake_minimum_required(VERSION 3.10) # I had problems if I only specified "VERSION 3".
project(calculator)

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

# Executables

reflex_add_source(${calculator_SOURCE_DIR}/calculator_scanner.reflex ${calculator_SOURCE_DIR} calculator_scanner ${PROJECT_BINARY_DIR})
trison_add_source(${calculator_SOURCE_DIR}/calculator_parser.trison ${calculator_SOURCE_DIR} calculator_parser ${PROJECT_BINARY_DIR})

set(calculator_SOURCES
    calculator.hpp
    calculator_main.cpp
    calculator_parser.cpp
    calculator_parser.hpp
    calculator_parser.trison
    calculator_scanner.cpp
    calculator_scanner.hpp
    calculator_scanner.reflex
)
add_executable(calculator ${calculator_SOURCES})
target_link_libraries(calculator PUBLIC Strict)
