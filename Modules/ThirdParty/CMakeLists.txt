cmake_minimum_required(VERSION 3.20)

project(IgnisThirdParty)

add_subdirectory(vkfw)
add_subdirectory(VulkanMemoryAllocator)
add_subdirectory(VulkanMemoryAllocator-Hpp)

find_path(GTL_INCLUDE_DIRS "gtl/adv_utils.hpp")

find_package(Stb REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(function2 CONFIG REQUIRED)
find_package(concurrentqueue CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fastgltf CONFIG REQUIRED)
find_package(Taskflow CONFIG REQUIRED)
find_package(slang CONFIG REQUIRED)

set(IGNIS_THIRD_PARTY_IMGUI_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imconfig.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_internal.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_rectpack.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_textedit.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_truetype.h

        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_vulkan.h
)

set(IGNIS_THIRD_PARTY_IMGUI_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_vulkan.cpp
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/imgui PREFIX Include FILES ${VULKAN_RENDER_THIRD_PARTY_IMGUI_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/imgui PREFIX Source FILES ${VULKAN_RENDER_THIRD_PARTY_IMGUI_SOURCES})

add_library(IgnisThirdParty STATIC
        ${IGNIS_THIRD_PARTY_IMGUI_HEADERS}
        ${IGNIS_THIRD_PARTY_IMGUI_SOURCES}
)
add_library(Ignis::ThirdParty ALIAS IgnisThirdParty)

target_link_libraries(IgnisThirdParty PUBLIC
        glfw
        vkfw
        glm::glm
        spdlog::spdlog
        function2::function2
        concurrentqueue::concurrentqueue
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
        VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
        Taskflow::Taskflow
        fastgltf::fastgltf
        # slang::slang
)

target_include_directories(IgnisThirdParty PUBLIC
        ${Stb_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui
        ${GTL_INCLUDE_DIRS}
)

set_property(TARGET IgnisThirdParty PROPERTY FOLDER "ThirdParty")

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(X11 REQUIRED)

    target_link_libraries(IgnisThirdParty PRIVATE
            X11::X11
            X11::Xcursor
            X11::Xi
            X11::Xrandr
            X11::Xinerama
    )
endif ()

