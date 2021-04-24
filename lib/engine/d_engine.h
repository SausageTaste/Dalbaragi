#pragma once

#include "d_vulkan_man.h"
#include "d_inputs.h"
#include "d_actor.h"
#include "d_timer.h"
#include "d_input_consumer.h"


namespace dal {

    struct EngineCreateInfo {
        const char*                     m_window_title = nullptr;
        unsigned                        m_init_width = 0;
        unsigned                        m_init_height = 0;
        dal::Filesystem*                m_filesystem = nullptr;
        std::vector<const char*>        m_extensions;
        std::function<void*(void*)>     m_surface_create_func;

        bool check_validity() const;
    };


    class Engine {

    private:
        dal::TaskManager m_task_man;
        dal::VulkanState m_vulkan_man;
        InputManager m_input_man;

        std::vector<IInputListener*> m_input_listeners;
        InputDispatcher m_input_dispatch;
        unsigned m_screen_width, m_screen_height;

        EulerCamera m_camera;
        Timer m_timer;

    public:
        Engine();

        Engine(const EngineCreateInfo& create_info);

        ~Engine();

        void init(const EngineCreateInfo& create_info);

        void destroy();

        void update();

        void wait_device_idle() const;

        void on_screen_resize(const unsigned width, const unsigned height);

        auto& input_manager() {
            return this->m_input_man;
        }

    };

}
