cmake_minimum_required(VERSION 3.11.0)

include(FetchContent)


FetchContent_Declare(
  fetch_fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        7.1.3
)
FetchContent_MakeAvailable(fetch_fmt)


add_library(libdal_util STATIC
    d_logger.h       d_logger.cpp
    d_log_channel.h  d_log_channel.cpp
    d_filesystem.h   d_filesystem.cpp
)
target_compile_features(libdal_util PUBLIC cxx_std_17)
target_include_directories(libdal_util PUBLIC ./)

target_link_libraries(libdal_util
    PUBLIC
        fmt
)
