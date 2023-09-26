cmake_minimum_required(VERSION 3.10.2)

project(Dalbaragi-Android)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")


add_library(dal_android_native_lib SHARED
    main.cpp
    filesys_android.h               filesys_android.cpp
)

target_compile_definitions(dal_android_native_lib PRIVATE VK_USE_PLATFORM_ANDROID_KHR)


find_library(log-lib log)

add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(native_app_glue
    PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)

add_subdirectory(./../../../../../../lib ${CMAKE_CURRENT_BINARY_DIR}/libdal)

target_link_libraries(dal_android_native_lib
    native_app_glue
    android
    ${log-lib}

    dalbaragi::util
    libdal_vulkan
    libdal_engine
)
