#include "d_engine.h"

#include "d_logger.h"


namespace dal {

    bool EngineCreateInfo::check_validity() const {
        if (nullptr == this->m_window_title)
            return false;
        if (0 == this->m_init_width)
            return false;
        if (0 == this->m_init_height)
            return false;
        if (nullptr == this->m_asset_mgr)
            return false;
        if (!this->m_surface_create_func)
            return false;

        return true;
    }

}


namespace dal {

    Engine::~Engine() {
        this->destroy();
    }

    void Engine::init(const EngineCreateInfo& create_info) {
        dalAssert(create_info.check_validity());

        this->destroy();

        this->m_vulkan_man.init(
            create_info.m_window_title,
            create_info.m_init_width,
            create_info.m_init_height,
            *create_info.m_asset_mgr,
            create_info.m_extensions,
            create_info.m_surface_create_func
        );
    }

    void Engine::destroy() {
        this->m_vulkan_man.destroy();
    }

    void Engine::update() {
        this->m_vulkan_man.update();
    }

    void Engine::wait_device_idle() const {
        this->m_vulkan_man.wait_device_idle();
    }

    void Engine::on_screen_resize(const unsigned width, const unsigned height) {
        this->m_vulkan_man.on_screen_resize(width, height);
    }

}
