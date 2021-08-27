#include <iostream>

#include "d_glfw.h"
#include "d_logger.h"
#include "d_engine.h"
#include "d_filesystem_std.h"


int main(int argc, char** argv) {
    for (int i = 0; i < argc; ++i) {
        std::cout << "Argument[" << i << "] " << argv[i] << std::endl;
    }

    dal::LoggerSingleton::inst().add_channel(dal::get_log_channel_cout());
    dal::Filesystem filesys;
    filesys.init(
        std::make_unique<dal::AssetManagerSTD>(),
        std::make_unique<dal::UserDataManagerSTD>()
    );

    dal::WindowGLFW window("Dalbrargi");

    dal::EngineCreateInfo engine_info;
    engine_info.m_window_title = "Dalbrargi";
    engine_info.m_filesystem = &filesys;
    engine_info.m_extensions = window.get_vulkan_extensions();

    dal::Engine engine{ engine_info };
    engine.init_vulkan(window.width(), window.height(), window.get_vk_surface_creator());

    window.set_callback_fbuf_resize([&engine](int width, int height) { engine.on_screen_resize(width, height); });
    window.set_callback_mouse_event([&engine](const dal::MouseEvent& e) {
        engine.input_manager().touch_manager().push_back(static_cast<dal::TouchEvent>(e));
    });
    window.set_callback_key_event([&engine](const dal::KeyEvent& e) {
        engine.input_manager().key_manager().push_back(e);
    });
    window.set_callback_gamepad_connection([&engine](const dal::GamepadConnectionEvent& e) {
        engine.input_manager().gamepad_manager().notify_connection_change(e);
    });

    dalInfo("Done init");

    while (!window.should_close()) {
        window.update_input_gamepad(engine.input_manager().gamepad_manager());
        window.do_frame();
        engine.update();
    }

    engine.wait_device_idle();
    return 0;
}
