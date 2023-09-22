cmake_minimum_required(VERSION 3.11.0)


add_subdirectory(./renderer)
add_subdirectory(./vulkan)
add_subdirectory(./engine)


if (WIN32)
    add_subdirectory(./window)

elseif ("${CMAKE_SYSTEM}" MATCHES "Linux")
    add_subdirectory(./window)

elseif (ANDROID)
endif()
