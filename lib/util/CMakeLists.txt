cmake_minimum_required(VERSION 3.4.1)


add_library(libdal_util STATIC
    d_logger.h              d_logger.cpp
    d_log_channel.h         d_log_channel.cpp
    d_filesystem.h          d_filesystem.cpp
    d_defines.h
    d_model_data.h          d_model_data.cpp
)
target_compile_features(libdal_util PUBLIC cxx_std_17)
target_include_directories(libdal_util PUBLIC ./)

target_link_libraries(libdal_util
    PUBLIC
        fmt
        glm::glm
)
