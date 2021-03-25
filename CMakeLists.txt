cmake_minimum_required(VERSION 3.4.1)

project(Dalbaragi)


add_subdirectory(./lib)

if (WIN32)
    add_subdirectory(./app/windows)

elseif ("${CMAKE_SYSTEM}" MATCHES "Linux")
    add_subdirectory(./app/linux)

endif()
