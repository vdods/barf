# Suite of tests for BARF

cmake_minimum_required(VERSION 3.10) # I had problems if I only specified "VERSION 3".
project(using_unique_ptr)

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

reflex_add_source(${using_unique_ptr_SOURCE_DIR}/using_unique_ptr_scanner.reflex ${using_unique_ptr_SOURCE_DIR} using_unique_ptr_scanner ${PROJECT_BINARY_DIR})
trison_add_source(${using_unique_ptr_SOURCE_DIR}/using_unique_ptr_parser.trison ${using_unique_ptr_SOURCE_DIR} using_unique_ptr_parser ${PROJECT_BINARY_DIR})

set(using_unique_ptr_SOURCES
    using_unique_ptr_ast.cpp
    using_unique_ptr_ast.hpp
    using_unique_ptr_main.cpp
    using_unique_ptr_parser.cpp
    using_unique_ptr_parser.hpp
    using_unique_ptr_parser.trison
    using_unique_ptr_scanner.cpp
    using_unique_ptr_scanner.hpp
    using_unique_ptr_scanner.reflex
)
add_executable(using_unique_ptr ${using_unique_ptr_SOURCES})
target_link_libraries(using_unique_ptr PUBLIC Strict)
