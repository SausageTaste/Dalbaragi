#pragma once

#include "d_resource_man.h"
#include "d_inputs.h"
#include "d_timer.h"
#include "d_input_consumer.h"
#include "d_scene.h"


namespace dal {

    struct EngineCreateInfo {
        std::string                     m_window_title;
        dal::Filesystem*                m_filesystem = nullptr;
        std::vector<std::string>        m_extensions;
        surface_create_func_t           m_surface_create_func;

        bool check_validity() const;
    };


    class Engine {

    private:
        EngineCreateInfo m_create_info;

        InputManager m_input_man;
        TaskManager m_task_man;
        ResourceManager m_res_man;

        Scene m_scene;
        std::unique_ptr<IRenderer> m_renderer;

        std::vector<IInputListener*> m_input_listeners;
        InputDispatcher m_input_dispatch;
        unsigned m_screen_width, m_screen_height;

        Timer m_timer;

    public:
        Engine(const EngineCreateInfo& create_info);

        ~Engine();

        void update();

        void init_vulkan(const unsigned win_width, const unsigned win_height);

        void destory_vulkan();

        void wait_device_idle() const;

        void on_screen_resize(const unsigned width, const unsigned height);

        auto& input_manager() {
            return this->m_input_man;
        }

    private:
        void init(const EngineCreateInfo& create_info);

        void destroy();

    };

}
