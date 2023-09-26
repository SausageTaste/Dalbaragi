cmake_minimum_required(VERSION 3.11.0)

include(FetchContent)


FetchContent_Declare(
    fetch_glfw
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG        3.3.6
)
option(GLFW_BUILD_DOCS off)
option(GLFW_BUILD_EXAMPLES off)
option(GLFW_BUILD_TESTS off)
option(GLFW_USE_HYBRID_HPG on)

FetchContent_MakeAvailable(fetch_glfw)
find_package(Vulkan REQUIRED)


add_library(libdal_glfw STATIC
    src/glfw.cpp
)
add_library(dal::glfw ALIAS libdal_glfw)
target_compile_features(libdal_glfw PUBLIC cxx_std_17)
target_include_directories(libdal_glfw PUBLIC ./include)

target_link_libraries(libdal_glfw PUBLIC
    glfw
    Vulkan::Vulkan
    libdal_engine
)
