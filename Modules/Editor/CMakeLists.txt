cmake_minimum_required(VERSION 3.20)

project(IgnisEditor LANGUAGES CXX VERSION 0.0.1)

#if(WIN32)
#    option(WIN32_RELEASE_MODE_NO_CONSOLE "Release mode for Win32" OFF)
#endif()

set(IGNIS_ASSETS_SOURCE_DIR ${CMAKE_SOURCE_DIR}/Assets)
set(IGNIS_ASSETS_DESTINATION_DIR ${CMAKE_BINARY_DIR}/Assets)

file(GLOB_RECURSE IGNIS_EDITOR_INCLUDE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/*.h
)
file(GLOB_RECURSE IGNIS_EDITOR_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
    ${IGNIS_EDITOR_INCLUDE_FILES}
    ${IGNIS_EDITOR_SOURCE_FILES}
)

add_executable(IgnisEditor
    ${IGNIS_EDITOR_INCLUDE_FILES}
    ${IGNIS_EDITOR_SOURCE_FILES}
)

target_include_directories(IgnisEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

target_link_libraries(IgnisEditor PRIVATE Ignis::Ignis)

target_precompile_headers(IgnisEditor REUSE_FROM Ignis::Ignis)

if(WIN32 AND WIN32_RELEASE_MODE_NO_CONSOLE)
    if(MSVC)
        set_target_properties(IgnisEditor PROPERTIES WIN32_EXECUTABLE $<$<CONFIG:Release>:TRUE>)
    elseif(CMAKE_BUILD_TYPE MATCHES Release)
        set_target_properties(IgnisEditor PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()

if(MSVC)
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT IgnisEditor)
endif()

find_program(SLANGC_EXECUTABLE slangc REQUIRED)

if(NOT SLANGC_EXECUTABLE)
    message(FATAL_ERROR "Slang compiler not found. Install slangc and add it to PATH.")
endif()

file(GLOB_RECURSE IGNIS_EDITOR_SHADERS ${IGNIS_ASSETS_SOURCE_DIR}/Shaders/*.slang)

set(SPIRV_BINARY_FILES "")

foreach(SHADER_FILE ${IGNIS_EDITOR_SHADERS})
    get_filename_component(FILE_NAME_WE ${SHADER_FILE} NAME_WE)
    get_filename_component(FILE_PATH ${SHADER_FILE} PATH)

    set(SPIRV ${FILE_PATH}/${FILE_NAME_WE}.spv)

    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${SLANGC_EXECUTABLE} -fvk-use-entrypoint-name -emit-spirv-directly -I ${IGNIS_ASSETS_SOURCE_DIR} ${SHADER_FILE} -target spirv -o ${SPIRV}
        DEPENDS ${SHADER_FILE}
    )

    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()

add_custom_target(IgnisEditorAssets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${IGNIS_ASSETS_SOURCE_DIR} ${IGNIS_ASSETS_DESTINATION_DIR}
    DEPENDS ${SPIRV_BINARY_FILES} ${IGNIS_ASSETS_SOURCE_DIR}
    COMMENT "Copying '${IGNIS_ASSETS_SOURCE_DIR}' folder to '${IGNIS_ASSETS_DESTINATION_DIR}'"
)

add_dependencies(IgnisEditor IgnisEditorAssets)

file(GLOB_RECURSE IGNIS_ASSETS_FILES ${IGNIS_ASSETS_SOURCE_DIR}/*)
source_group(TREE ${IGNIS_ASSETS_SOURCE_DIR} PREFIX Assets FILES ${IGNIS_ASSETS_FILES})
target_sources(IgnisEditorAssets PRIVATE ${IGNIS_ASSETS_FILES})

set_property(TARGET IgnisEditorAssets PROPERTY FOLDER "Editor")
set_property(TARGET IgnisEditor PROPERTY FOLDER "Editor")