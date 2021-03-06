cmake_minimum_required(VERSION 3.4.1)


add_library(libdal_util STATIC
    d_logger.h              d_logger.cpp
    d_log_channel.h         d_log_channel.cpp
    d_filesystem.h          d_filesystem.cpp
    d_defines.h
    d_model_data.h          d_model_data.cpp
    d_image_parser.h        d_image_parser.cpp
    d_timer.h               d_timer.cpp
    d_model_parser.h        d_model_parser.cpp
    d_actor.h               d_actor.cpp
    d_task_thread.h         d_task_thread.cpp
    d_input_consumer.h      d_input_consumer.cpp
    d_input_def.h           d_input_def.cpp
    d_static_list.h         d_static_list.cpp
    d_konsts.h
    d_animation.h           d_animation.cpp
    d_indices.h             d_indices.cpp
)
target_compile_features(libdal_util PUBLIC cxx_std_17)
target_include_directories(libdal_util PUBLIC ./)

target_link_libraries(libdal_util
    PUBLIC
        fmt
        glm::glm
        dalbaragi_model_parser
)
target_include_directories(libdal_util
    PRIVATE
        ${fetch_stb_SOURCE_DIR}
)
