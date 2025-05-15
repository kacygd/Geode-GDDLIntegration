cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(GDDLIntegration VERSION 1.0.0)

file(GLOB_RECURSE SOURCES
    src/*.cpp src/*.h src/*/*.cpp src/*/*.h src/*/*/*.cpp src/*/*/*.h
)

include_directories(src)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=return-type")
set(CMAKE_CXX_FLAGS_RELEASE "-O0") # Disable optimizations for testing

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME} EXTERNALS geode.node-ids:1.13.2)

if (WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,--export-all-symbols")
endif()
