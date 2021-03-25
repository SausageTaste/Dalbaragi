#pragma once


// https://stackoverflow.com/questions/5919996/how-to-detect-reliably-mac-os-x-ios-linux-windows-in-c-preprocessor
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #define DAL_OS_WINDOWS

    #ifdef _WIN64
        #define DAL_OS_WINDOWS_X64
    #else
        #define DAL_OS_WINDOWS_X32
    #endif
#elif __ANDROID__
    #define DAL_OS_ANDROID
#elif __APPLE__
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR
         // iOS Simulator
    #elif TARGET_OS_IPHONE
        // iOS device
    #elif TARGET_OS_MAC
        // Other kinds of Mac OS
    #else
    #   error "Unknown Apple platform"
    #endif
#elif __linux__
    #define DAL_LINUX
#elif __unix__ // all unices not caught above
    #define DAL_UNIX
#elif defined(_POSIX_VERSION)
    // POSIX
#else
#   error "Unknown compiler"
#endif
