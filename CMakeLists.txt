cmake_minimum_required(VERSION 3.4.1)


set(extern_dir ./extern)


if (WIN32)
    find_package(Vulkan REQUIRED)
    add_subdirectory(${extern_dir}/glfw ${CMAKE_CURRENT_BINARY_DIR}/glfw)

endif()


add_subdirectory(./lib/vulkan)


if (WIN32)
    add_subdirectory(./lib/window)
    add_subdirectory(./app/windows)

endif()
