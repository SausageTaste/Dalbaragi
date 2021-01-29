cmake_minimum_required(VERSION 3.4.1)

project(LibDalbaragi_Vulkan
    LANGUAGES CXX
)


add_library(libdal_vulkan STATIC
    d_vulkan_man.h       d_vulkan_man.cpp
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
    target_compile_definitions(libdal_vulkan PUBLIC VK_USE_PLATFORM_WIN32_KHR)

elseif (ANDROID)
    target_compile_definitions(libdal_vulkan PUBLIC VK_USE_PLATFORM_ANDROID_KHR)

endif()
