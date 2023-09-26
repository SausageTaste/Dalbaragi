cmake_minimum_required(VERSION 3.4.1)


add_library(libdal_util STATIC
    src/actor.cpp
    src/animation.cpp
    src/collider.cpp
    src/filesystem.cpp
    src/filesystem_std.cpp
    src/geometry.cpp
    src/image_parser.cpp
    src/indices.cpp
    src/input_consumer.cpp
    src/input_def.cpp
    src/json_util.cpp
    src/log_channel.cpp
    src/logger.cpp
    src/mesh_builder.cpp
    src/model_data.cpp
    src/static_list.cpp
    src/task_thread.cpp
)
add_library(dalbaragi::util ALIAS libdal_util)
target_compile_features(libdal_util PUBLIC cxx_std_17)
target_include_directories(libdal_util PUBLIC ./include)

target_link_libraries(libdal_util
    PUBLIC
        fmt
        glm::glm
        dalbaragi::daltools
        nlohmann_json::nlohmann_json
)
target_include_directories(libdal_util
    PRIVATE
        ${fetch_stb_SOURCE_DIR}
)
