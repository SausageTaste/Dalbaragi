cmake_minimum_required(VERSION 3.4.1)


add_executable(dalbaragi_linux
    main.cpp
)
target_compile_features(dalbaragi_linux PUBLIC cxx_std_17)

target_link_libraries(dalbaragi_linux
    PRIVATE
        libdal_window
)
