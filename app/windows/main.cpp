#include <iostream>

#include "d_glfw.h"
#include "d_vulkan_man.h"
#include "d_logger.h"


namespace {

    class LogChannel_COUT : public dal::ILogChannel {

    public:
        virtual void put(
            const dal::LogLevel level, const char* const str,
            const int line, const char* const func, const char* const file
        ) override {
            if (static_cast<int>(level) >= static_cast<int>(dal::LogLevel::warning))
                std::cerr << "[" << dal::get_log_level_str(level) << "] " << str << std::endl;
            else
                std::cout << "[" << dal::get_log_level_str(level) << "] " << str << std::endl;
        }

    };

}


int main(int argc, char** argv) {
    for (int i = 0; i < argc; ++i) {
        std::cout << "Argument[" << i << "] " << argv[i] << std::endl;
    }

    dal::LoggerSingleton::inst().emplace_channel<LogChannel_COUT>();

    dal::WindowGLFW window("Dalbrargi Windows");
    dal::VulkanState state;
    state.init("Dalbrargi Windows", window.width(), window.height(), window.get_vulkan_extensions(), window.get_vk_surface_creator());

    while (!window.should_close()) {
        window.do_frame();
    }

    return 0;
}
