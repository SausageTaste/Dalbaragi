cmake_minimum_required(VERSION 3.4.1)


if (WIN32)

    add_subdirectory(./lib/window)
    add_subdirectory(./app/windows)

endif()
