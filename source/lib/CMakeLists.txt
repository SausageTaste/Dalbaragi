cmake_minimum_required(VERSION 3.11.0)

include(FetchContent)

set(extern_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../extern)


add_definitions(-D_CRT_SECURE_NO_WARNINGS)


# Define external libraries
#-------------------------------------------------------------------------------

FetchContent_Declare(
    fetch_fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG        7.1.3
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
    fetch_daltools
    GIT_REPOSITORY https://github.com/SausageTaste/DalbaragiTools
    GIT_TAG        b63306b38747ea26762dd250d52239abd9cd2269
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
FetchContent_MakeAvailable(fetch_stb)
FetchContent_MakeAvailable(fetch_entt)
FetchContent_MakeAvailable(fetch_daltools)
FetchContent_MakeAvailable(fetch_lua)
FetchContent_MakeAvailable(fetch_json)

add_subdirectory(${extern_dir}/glslang ${CMAKE_CURRENT_BINARY_DIR}/glslang)
add_subdirectory(${extern_dir}/SPIRV-Headers ${CMAKE_CURRENT_BINARY_DIR}/SPIRV-Headers)
add_subdirectory(${extern_dir}/SPIRV-Tools ${CMAKE_CURRENT_BINARY_DIR}/SPIRV-Tools)
add_subdirectory(${extern_dir}/shaderc ${CMAKE_CURRENT_BINARY_DIR}/shaderc)

if (WIN32)
    find_package(Vulkan REQUIRED)
elseif ("${CMAKE_SYSTEM}" MATCHES "Linux")
    find_package(Vulkan REQUIRED)
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

add_subdirectory(util)
add_subdirectory(engine)
