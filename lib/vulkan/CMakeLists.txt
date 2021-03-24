cmake_minimum_required(VERSION 3.4.1)

project(LibDalbaragi_Vulkan
    LANGUAGES CXX
)


add_library(libdal_vulkan STATIC
    d_vulkan_man.h       d_vulkan_man.cpp
    d_vulkan_header.h
    d_swapchain.h        d_swapchain.cpp
    d_image_obj.h        d_image_obj.cpp
    d_shader.h           d_shader.cpp
    d_render_pass.h      d_render_pass.cpp
    d_framebuffer.h      d_framebuffer.cpp
)
target_compile_features(libdal_vulkan PUBLIC cxx_std_17)
target_include_directories(libdal_vulkan PUBLIC ./)

if (ANDROID)
    target_sources(libdal_vulkan
    PRIVATE
        vulkan_wrapper.h     vulkan_wrapper.cpp
    )
endif()


# Link libraries

target_link_libraries(libdal_vulkan PUBLIC libdal_util)


if(WIN32)
    target_link_libraries(libdal_vulkan PUBLIC Vulkan::Vulkan)
    target_compile_definitions(libdal_vulkan PRIVATE VK_USE_PLATFORM_WIN32_KHR)

elseif ("${CMAKE_SYSTEM}" MATCHES "Linux")
    target_link_libraries(libdal_vulkan PUBLIC Vulkan::Vulkan)

elseif (ANDROID)
    target_compile_definitions(libdal_vulkan PUBLIC VK_USE_PLATFORM_ANDROID_KHR)

endif()
