cmake_minimum_required(VERSION 3.4.1)


add_library(libdal_renderer STATIC
    d_renderer.h                d_renderer.cpp
    d_renderer_create.h         d_renderer_create.cpp
    d_render_cpnt.h             d_render_cpnt.cpp
    d_render_config.h           d_render_config.cpp
    d_scene.h                   d_scene.cpp
)
target_compile_features(libdal_renderer PUBLIC cxx_std_17)
target_include_directories(libdal_renderer PUBLIC ./)

target_link_libraries(libdal_renderer
    PUBLIC
        EnTT::EnTT
        libdal_util

    PRIVATE
        libdal_vulkan
)
