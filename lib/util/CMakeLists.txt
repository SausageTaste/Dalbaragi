cmake_minimum_required(VERSION 3.4.1)


add_library(libdal_util STATIC
    d_logger.h       d_logger.cpp
)
target_compile_features(libdal_util PUBLIC cxx_std_17)
target_include_directories(libdal_util PUBLIC ./)

target_link_libraries(libdal_util
    PUBLIC
        fmt
)
