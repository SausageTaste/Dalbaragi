cmake_minimum_required(VERSION 3.11.0)

include(FetchContent)

set(extern_dir ./../extern)


# Define external libraries
#-------------------------------------------------------------------------------

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
option(GLFW_BUILD_DOCS off)
option(GLFW_BUILD_EXAMPLES off)
option(GLFW_BUILD_TESTS off)
option(GLFW_USE_HYBRID_HPG on)

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

FetchContent_Declare(
    fetch_entt
    GIT_REPOSITORY https://github.com/skypjack/entt
    GIT_TAG        v3.7.1
)

FetchContent_Declare(
    fetch_dal_blender
    GIT_REPOSITORY https://github.com/SausageTaste/io_scene_dalbaragi
    GIT_TAG        main
)

FetchContent_Declare(
    fetch_lua
    GIT_REPOSITORY https://github.com/lua/lua
    GIT_TAG        v5.4.3
)

FetchContent_Declare(
    fetch_json
    GIT_REPOSITORY https://github.com/nlohmann/json
    GIT_TAG        v3.10.2
)

# SPIRV-Tools
set(SPIRV_SKIP_EXECUTABLES on)
set(SPIRV_SKIP_TESTS on)

# shaderc
set(SHADERC_ENABLE_SHARED_CRT on)
set(SHADERC_SKIP_EXAMPLES on)
set(SHADERC_SKIP_INSTALL on)
set(SHADERC_SKIP_TESTS on)


# Import libraries
#-------------------------------------------------------------------------------

FetchContent_MakeAvailable(fetch_fmt)
FetchContent_MakeAvailable(fetch_glm)
FetchContent_MakeAvailable(fetch_stb)
FetchContent_MakeAvailable(fetch_entt)
FetchContent_MakeAvailable(fetch_dal_blender)
FetchContent_MakeAvailable(fetch_lua)
FetchContent_MakeAvailable(fetch_json)

add_subdirectory(${extern_dir}/glslang ${CMAKE_CURRENT_BINARY_DIR}/glslang)
add_subdirectory(${extern_dir}/SPIRV-Headers ${CMAKE_CURRENT_BINARY_DIR}/SPIRV-Headers)
add_subdirectory(${extern_dir}/SPIRV-Tools ${CMAKE_CURRENT_BINARY_DIR}/SPIRV-Tools)
add_subdirectory(${extern_dir}/shaderc ${CMAKE_CURRENT_BINARY_DIR}/shaderc)

if (WIN32)
    find_package(Vulkan REQUIRED)
    FetchContent_MakeAvailable(fetch_glfw)

elseif ("${CMAKE_SYSTEM}" MATCHES "Linux")
    find_package(Vulkan REQUIRED)
    FetchContent_MakeAvailable(fetch_glfw)

elseif (ANDROID)
endif()


add_library(lua_static STATIC
    ${fetch_lua_SOURCE_DIR}/lapi.c
    ${fetch_lua_SOURCE_DIR}/lcode.c
    ${fetch_lua_SOURCE_DIR}/lctype.c
    ${fetch_lua_SOURCE_DIR}/ldebug.c
    ${fetch_lua_SOURCE_DIR}/ldo.c
    ${fetch_lua_SOURCE_DIR}/ldump.c
    ${fetch_lua_SOURCE_DIR}/lfunc.c
    ${fetch_lua_SOURCE_DIR}/lgc.c
    ${fetch_lua_SOURCE_DIR}/llex.c
    ${fetch_lua_SOURCE_DIR}/lmem.c
    ${fetch_lua_SOURCE_DIR}/lobject.c
    ${fetch_lua_SOURCE_DIR}/lopcodes.c
    ${fetch_lua_SOURCE_DIR}/lparser.c
    ${fetch_lua_SOURCE_DIR}/lstate.c
    ${fetch_lua_SOURCE_DIR}/lstring.c
    ${fetch_lua_SOURCE_DIR}/ltable.c
    ${fetch_lua_SOURCE_DIR}/ltm.c
    ${fetch_lua_SOURCE_DIR}/lundump.c
    ${fetch_lua_SOURCE_DIR}/lvm.c
    ${fetch_lua_SOURCE_DIR}/lzio.c
    ${fetch_lua_SOURCE_DIR}/lauxlib.c
    ${fetch_lua_SOURCE_DIR}/lbaselib.c
    ${fetch_lua_SOURCE_DIR}/lcorolib.c
    ${fetch_lua_SOURCE_DIR}/ldblib.c
    ${fetch_lua_SOURCE_DIR}/liolib.c
    ${fetch_lua_SOURCE_DIR}/lmathlib.c
    ${fetch_lua_SOURCE_DIR}/loadlib.c
    ${fetch_lua_SOURCE_DIR}/loslib.c
    ${fetch_lua_SOURCE_DIR}/lstrlib.c
    ${fetch_lua_SOURCE_DIR}/ltablib.c
    ${fetch_lua_SOURCE_DIR}/lutf8lib.c
    ${fetch_lua_SOURCE_DIR}/linit.c
)
target_include_directories(lua_static PUBLIC ${fetch_lua_SOURCE_DIR})


# Add project directories
#-------------------------------------------------------------------------------

add_subdirectory(./util)
add_subdirectory(./renderer)
add_subdirectory(./vulkan)
add_subdirectory(./engine)


if (WIN32)
    add_subdirectory(./window)

elseif ("${CMAKE_SYSTEM}" MATCHES "Linux")
    add_subdirectory(./window)

elseif (ANDROID)
endif()
