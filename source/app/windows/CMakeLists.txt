cmake_minimum_required(VERSION 3.4.1)


add_executable(dalbaragi_windows
    main.cpp
)
target_compile_features(dalbaragi_windows PUBLIC cxx_std_17)

target_link_libraries(dalbaragi_windows
    PRIVATE
        libdal_window
        libdal_engine
)
