cmake_minimum_required(VERSION 3.4.1)


add_library(libdal_renderer STATIC
    d_renderer.h                d_renderer.cpp
)
target_compile_features(libdal_renderer PUBLIC cxx_std_17)
target_include_directories(libdal_renderer PUBLIC ./)

target_link_libraries(libdal_renderer
    PUBLIC
        libdal_util
        libdal_vulkan
)
