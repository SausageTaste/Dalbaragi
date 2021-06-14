cmake_minimum_required(VERSION 3.4.1)


add_library(libdal_scene STATIC
    d_scene.h                d_scene.cpp
)
target_compile_features(libdal_scene PUBLIC cxx_std_17)
target_include_directories(libdal_scene PUBLIC ./)

target_link_libraries(libdal_scene
    PUBLIC
        EnTT::EnTT
        libdal_util
        libdal_renderer
)
