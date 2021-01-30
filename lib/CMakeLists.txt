cmake_minimum_required(VERSION 3.4.1)


set(extern_dir ./../extern)


if (WIN32)
    find_package(Vulkan REQUIRED)
    add_subdirectory(${extern_dir}/glfw ${CMAKE_CURRENT_BINARY_DIR}/glfw)

elseif ("${CMAKE_SYSTEM}" MATCHES "Linux")
    find_package(Vulkan REQUIRED)
    add_subdirectory(${extern_dir}/glfw ${CMAKE_CURRENT_BINARY_DIR}/glfw)

elseif (ANDROID)

endif()


add_subdirectory(./util)
add_subdirectory(./vulkan)


if (WIN32)
    add_subdirectory(./window)

elseif ("${CMAKE_SYSTEM}" MATCHES "Linux")
    add_subdirectory(./window)

elseif (ANDROID)
endif()
