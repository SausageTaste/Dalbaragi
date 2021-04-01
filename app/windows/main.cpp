#include <iostream>

#include "d_glfw.h"
#include "d_vulkan_man.h"
#include "d_logger.h"
#include "d_filesystem.h"


int main(int argc, char** argv) {
    for (int i = 0; i < argc; ++i) {
        std::cout << "Argument[" << i << "] " << argv[i] << std::endl;
    }

    dal::LoggerSingleton::inst().add_channel(dal::get_log_channel_cout());
    dal::filesystem::AssetManager asset_mgr;

    dal::WindowGLFW window("Dalbrargi Windows");
    dal::VulkanState state(
        "Dalbrargi Windows",
        window.width(),
        window.height(),
        asset_mgr,
        window.get_vulkan_extensions(),
        window.get_vk_surface_creator()
    );

    window.set_callback_fbuf_resize([&state](int width, int height) { state.on_screen_resize(width, height); });

    dalInfo("Done init");

    while (!window.should_close()) {
        window.do_frame();
        state.update();
    }

    state.wait_device_idle();
    return 0;
}
