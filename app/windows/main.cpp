#include <iostream>

#include "d_glfw.h"


int main(int argc, char** argv) {
    for (int i = 0; i < argc; ++i) {
        std::cout << "Argument[" << i << "] " << argv[i] << std::endl;
    }

    dal::WindowGLFW window;

    return 0;
}
