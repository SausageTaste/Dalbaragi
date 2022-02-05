#pragma once

#include <mutex>

#include <daltools/util.h>

#include "d_resource_man.h"
#include "d_inputs.h"
#include "d_input_consumer.h"
#include "d_scene.h"
#include "d_script_engine.h"
#include "d_logger.h"
#include "d_config.h"


namespace dal {

    class LogChannel_FileOutput : public dal::ILogChannel {

    private:
        const int FLUSH_SIZE = 1024;

    private:
        std::mutex m_mut;
        std::string m_buffer;
        dal::Filesystem& m_filesys;

    public:
        LogChannel_FileOutput(dal::Filesystem& filesys);

        ~LogChannel_FileOutput();

        void put(
            const dal::LogLevel level, const char* const str,
            const int line, const char* const func, const char* const file
        ) override;

        void flush() override;

    };


    struct EngineCreateInfo {
        std::string                     m_window_title;
        dal::Filesystem*                m_filesystem = nullptr;

        bool check_validity() const;
    };


    class Engine {

    private:
        EngineCreateInfo m_create_info;

    public:
        MainConfig m_config;
        RendererConfig m_render_config;

    private:
        crypto::PublicKeySignature m_sign_mgr;

        LuaState m_lua;
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

        void init_vulkan(
            const unsigned win_width,
            const unsigned win_height,
            const surface_create_func_t surface_create_func,
            const std::vector<std::string>& extensions
        );

        void destory_vulkan();

        void wait_device_idle() const;

        void on_screen_resize(const unsigned width, const unsigned height);

        auto& input_manager() {
            return this->m_input_man;
        }

    };

}
