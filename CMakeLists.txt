cmake_minimum_required(VERSION 3.20)

list(APPEND CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_CMAKE})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC)
else()
    if(NOT DEFINED CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type: Debug or Release")
    endif()

    string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER)

    if(NOT CMAKE_BUILD_TYPE_UPPER MATCHES "^(DEBUG|RELEASE)$")
        message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE}")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_BUILD_TYPE Debug)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_BUILD_TYPE Release)
    endif()
endif()

project(Ignis LANGUAGES CXX C VERSION 0.0.1)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(WITHOUT_SAMPLES "Build without samples" OFF)

if(BUILD_SHARED_LIBS AND WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if(MSVC)
    add_compile_options(/MP)

    add_compile_definitions(
            $<$<CONFIG:Release>:IGNIS_BUILD_TYPE_RELEASE>
            $<$<CONFIG:Debug>:IGNIS_BUILD_TYPE_DEBUG>
    )

    add_compile_options(
            $<$<CONFIG:Release>:/O2>
            $<$<CONFIG:Release>:/Qpar>
            $<$<CONFIG:Debug>:/Zi>
            $<$<CONFIG:Debug>:/Od>
    )
else()
    if(CMAKE_BUILD_TYPE MATCHES Release)
        add_compile_definitions(IGNIS_BUILD_TYPE_RELEASE)
        add_compile_options(-O3 -funroll-loops)
    else()
        add_compile_definitions(IGNIS_BUILD_TYPE_DEBUG)
        add_compile_options(-g -pthread)
    endif()
endif()

add_compile_definitions(IGNIS_PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")
add_subdirectory(Modules)