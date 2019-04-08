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
