cmake_minimum_required(VERSION 3.4.1)


add_library(libdal_util STATIC
    d_logger.h              d_logger.cpp
    d_log_channel.h         d_log_channel.cpp
    d_filesystem.h          d_filesystem.cpp
    d_defines.h
    d_model_data.h          d_model_data.cpp
    d_image_parser.h        d_image_parser.cpp
    d_actor.h               d_actor.cpp
    d_task_thread.h         d_task_thread.cpp
    d_input_consumer.h      d_input_consumer.cpp
    d_input_def.h           d_input_def.cpp
    d_static_list.h         d_static_list.cpp
    d_konsts.h
    d_animation.h           d_animation.cpp
    d_indices.h             d_indices.cpp
    d_filesystem_std.h      d_filesystem_std.cpp
    d_json_util.h           d_json_util.cpp
    d_geometry.h            d_geometry.cpp
    d_mesh_builder.h        d_mesh_builder.cpp
    d_collider.h            d_collider.cpp
)
target_compile_features(libdal_util PUBLIC cxx_std_17)
target_include_directories(libdal_util PUBLIC ./)

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
