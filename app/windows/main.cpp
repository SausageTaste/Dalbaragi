#include <iostream>

#include <fmt/format.h>

#include "d_glfw.h"
#include "d_logger.h"
#include "d_engine.h"
#include "d_filesystem_std.h"


#define DAL_CATCH_EXCEPTION false


int main(int argc, char** argv) {
    for (int i = 0; i < argc; ++i) {
        std::cout << "Argument[" << i << "] " << argv[i] << std::endl;
    }

    dal::Filesystem filesys;
    filesys.init(
        std::make_unique<dal::AssetManagerSTD>(),
        std::make_unique<dal::UserDataManagerSTD>(),
        std::make_unique<dal::InternalManagerSTD>()
    );

    dal::LoggerSingleton::inst().add_channel(dal::get_log_channel_cout());
    dal::LoggerSingleton::inst().add_channel(std::make_shared<dal::LogChannel_FileOutput>(filesys));

    dal::EngineCreateInfo engine_info;
    engine_info.m_window_title = dal::APP_NAME;
    engine_info.m_filesystem = &filesys;

    dal::Engine engine{ engine_info };

    dal::WindowGLFW window(
        dal::APP_NAME,
        engine.m_config.m_full_screen,
        engine.m_config.m_window_width,
        engine.m_config.m_window_height
    );

    engine.init_vulkan(
        window.width(),
        window.height(),
        window.get_vk_surface_creator(),
        window.get_vulkan_extensions()
    );

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

#if DAL_CATCH_EXCEPTION
    try {
#endif
        while (!window.should_close()) {
            window.update_input_gamepad(engine.input_manager().gamepad_manager());
            window.do_frame();
            engine.update();
        }
#if DAL_CATCH_EXCEPTION
    }
    catch (const std::exception& e) {
        dalAbort(fmt::format("An exception caught: {}", e.what()).c_str());
    }
    catch (...) {
        dalAbort("Unknown exception caught");
    }
#endif

    dal::LoggerSingleton::inst().flush();

    return 0;
}
