cmake_minimum_required(VERSION 3.4.1)


add_library(libdal_engine STATIC
    d_engine.h                  d_engine.cpp
    d_inputs.h                  d_inputs.cpp
    d_resource_man.h            d_resource_man.cpp
    d_script_engine.h           d_script_engine.cpp
    d_config.h                  d_config.cpp
)

target_compile_features(libdal_engine PUBLIC cxx_std_17)

target_include_directories(libdal_engine
    PUBLIC
        .
    PRIVATE
        ${fetch_stb_SOURCE_DIR}
)

target_link_libraries(libdal_engine
    PUBLIC
        libdal_scene
        libdal_renderer
        nlohmann_json::nlohmann_json
    PRIVATE
        lua_static
)
