#pragma once

#include "d_renderer.h"
#include "d_filesystem.h"
#include "d_task_thread.h"
#include "d_inputs.h"
#include "d_timer.h"
#include "d_input_consumer.h"
#include "d_scene.h"


namespace dal {

    struct EngineCreateInfo {
        std::string                     m_window_title;
        dal::Filesystem*                m_filesystem = nullptr;
        std::vector<std::string>        m_extensions;
        std::function<void*(void*)>     m_surface_create_func;

        bool check_validity() const;
    };


    class Engine {

    private:
        EngineCreateInfo m_create_info;

        Scene m_scene;
        TaskManager m_task_man;
        std::unique_ptr<IRenderer> m_renderer;
        InputManager m_input_man;

        std::vector<IInputListener*> m_input_listeners;
        InputDispatcher m_input_dispatch;
        unsigned m_screen_width, m_screen_height;

        Timer m_timer;

    public:
        Engine();

        Engine(const EngineCreateInfo& create_info);

        ~Engine();

        void init(const EngineCreateInfo& create_info);

        void destroy();

        void update();

        void init_vulkan(const unsigned win_width, const unsigned win_height);

        void destory_vulkan();

        void wait_device_idle() const;

        void on_screen_resize(const unsigned width, const unsigned height);

        auto& input_manager() {
            return this->m_input_man;
        }

    };

}
