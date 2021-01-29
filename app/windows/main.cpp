#include <iostream>

#include "d_glfw.h"
#include "d_vulkan_man.h"


int main(int argc, char** argv) {
    for (int i = 0; i < argc; ++i) {
        std::cout << "Argument[" << i << "] " << argv[i] << std::endl;
    }

    dal::WindowGLFW window("Dalbrargi Windows");
    dal::VulkanState state;
    state.init("Dalbrargi Windows", {});

    while (!window.should_close()) {
        window.do_frame();
    }

    return 0;
}
