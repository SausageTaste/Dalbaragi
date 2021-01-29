cmake_minimum_required(VERSION 3.4.1)

project(LibDalbaragi_Window
    LANGUAGES CXX
)


add_library(libdal_window STATIC
    d_glfw.h       d_glfw.cpp
)
target_compile_features(libdal_window PUBLIC cxx_std_17)
target_include_directories(libdal_window PUBLIC ./)


# Link libraries

set(extern_dir ./../../extern)


if (NOT TARGET glfw)
    add_subdirectory(${extern_dir}/glfw ${CMAKE_CURRENT_BINARY_DIR}/glfw)
endif()

target_link_libraries(libdal_window
    PRIVATE
        glfw
)
