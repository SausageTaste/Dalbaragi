cmake_minimum_required(VERSION 3.4.1)


add_library(libdal_engine STATIC
    d_engine.h               d_engine.cpp
    d_inputs.h               d_inputs.cpp
)
target_compile_features(libdal_engine PUBLIC cxx_std_17)
target_include_directories(libdal_engine PUBLIC ./)

target_link_libraries(libdal_engine
    PUBLIC
        libdal_vulkan
)
target_include_directories(libdal_engine
    PRIVATE
        ${fetch_stb_SOURCE_DIR}
)
