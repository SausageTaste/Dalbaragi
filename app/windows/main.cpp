#include <iostream>

#include "d_glfw.h"
#include "d_logger.h"
#include "d_engine.h"


int main(int argc, char** argv) {
    for (int i = 0; i < argc; ++i) {
        std::cout << "Argument[" << i << "] " << argv[i] << std::endl;
    }

    dal::LoggerSingleton::inst().add_channel(dal::get_log_channel_cout());
    dal::filesystem::AssetManager asset_mgr;

    dal::WindowGLFW window("Dalbrargi Windows");

    dal::EngineCreateInfo engine_info;
    engine_info.m_window_title = "Dalbrargi Windows";
    engine_info.m_init_width = window.width();
    engine_info.m_init_height = window.height();
    engine_info.m_asset_mgr = &asset_mgr;
    engine_info.m_extensions = window.get_vulkan_extensions();
    engine_info.m_surface_create_func = window.get_vk_surface_creator();

    dal::Engine engine{ engine_info };
    window.set_callback_fbuf_resize([&engine](int width, int height) { engine.on_screen_resize(width, height); });
    window.set_callback_mouse_event([&engine](const dal::MouseEvent& e) {
        engine.input_manager().touch_manager().push_back(static_cast<dal::TouchEvent>(e));
    });

    dalInfo("Done init");

    while (!window.should_close()) {
        window.do_frame();
        engine.update();
    }

    engine.wait_device_idle();
    return 0;
}
