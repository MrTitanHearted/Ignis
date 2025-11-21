cmake_minimum_required(VERSION 3.20)

project(Ignis LANGUAGES CXX VERSION 0.0.1)

file(GLOB_RECURSE IGNIS_INCLUDE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/*.h
)
file(GLOB_RECURSE IGNIS_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
    ${IGNIS_INCLUDE_FILES}
    ${IGNIS_SOURCE_FILES}
)

if (BUILD_SHARED_LIBS)
    set(IGNIS_TYPE SHARED)
else ()
    set(IGNIS_TYPE STATIC)
endif ()

add_library(Ignis ${IGNIS_TYPE}
    ${IGNIS_INCLUDE_FILES}
    ${IGNIS_SOURCE_FILES}
)
add_library(Ignis::Ignis ALIAS Ignis)

target_include_directories(Ignis PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

target_link_libraries(Ignis PUBLIC
    Ignis::ThirdParty
)

target_compile_definitions(Ignis PUBLIC
    VULKAN_HPP_NO_EXCEPTIONS
    GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
    VKFW_NO_EXCEPTIONS
)

target_precompile_headers(Ignis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include/Ignis/Core/PCH.hpp)

set_property(TARGET Ignis PROPERTY FOLDER "Engine")