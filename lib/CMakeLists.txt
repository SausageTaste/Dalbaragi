cmake_minimum_required(VERSION 3.11.0)

include(FetchContent)

set(extern_dir ./../extern)


FetchContent_Declare(
    fetch_fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG        7.1.3
)

FetchContent_Declare(
  fetch_glfw
  GIT_REPOSITORY https://github.com/glfw/glfw
  GIT_TAG        3.3.2
)

FetchContent_Declare(
  fetch_glm
  GIT_REPOSITORY https://github.com/g-truc/glm
  GIT_TAG        0.9.9.8
)

FetchContent_Declare(
    fetch_stb
    GIT_REPOSITORY    https://github.com/nothings/stb
    GIT_TAG           master
    CONFIGURE_COMMAND ""
    BUILD_COMMAND     ""
)


FetchContent_MakeAvailable(fetch_fmt)
FetchContent_MakeAvailable(fetch_glm)
FetchContent_MakeAvailable(fetch_stb)

if (WIN32)
    find_package(Vulkan REQUIRED)
    FetchContent_MakeAvailable(fetch_glfw)

elseif ("${CMAKE_SYSTEM}" MATCHES "Linux")
    find_package(Vulkan REQUIRED)
    FetchContent_MakeAvailable(fetch_glfw)

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
