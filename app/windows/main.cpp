#include <iostream>

#include "d_glfw.h"
#include "d_vulkan_man.h"
#include "d_logger.h"


namespace {

    void simple_print(const char* const text) {
        std::cout << text << std::endl;
    }

}


int main(int argc, char** argv) {
    for (int i = 0; i < argc; ++i) {
        std::cout << "Argument[" << i << "] " << argv[i] << std::endl;
    }

    dal::LoggerSingleton::inst().add_print_func(simple_print);

    dal::WindowGLFW window("Dalbrargi Windows");
    dal::VulkanState state;
    state.init("Dalbrargi Windows", window.get_vulkan_extensions(), window.get_vk_surface_creator());

    while (!window.should_close()) {
        window.do_frame();
    }

    return 0;
}
