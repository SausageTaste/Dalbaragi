#pragma once

#include <string>
#include <vector>
#include <functional>

#include "d_inputs.h"
#include "d_renderer.h"


namespace dal {

    class WindowGLFW {

    private:
        void* m_window;
        std::string m_title;

        uint32_t m_windowed_width;
        uint32_t m_windowed_height;
        int32_t m_windowed_xpos;
        int32_t m_windowed_ypos;

    public:
        WindowGLFW(const char* const title);

        ~WindowGLFW();

        void do_frame();

        bool should_close() const;

        std::vector<std::string> get_vulkan_extensions() const;

        surface_create_func_t get_vk_surface_creator() const;

        bool is_fullscreen() const;

        void set_fullscreen(const bool fullscreen);

        void toggle_fullscreen() {
            this->set_fullscreen(!this->is_fullscreen());
        }

        void set_callback_fbuf_resize(std::function<void(int, int)> func);

        void set_callback_mouse_event(std::function<void(const dal::MouseEvent&)> func);

        void set_callback_key_event(std::function<void(const dal::KeyEvent&)> func);

        void set_callback_gamepad_connection(std::function<void(const dal::GamepadConnectionEvent&)> func);

        void update_input_gamepad(GamepadInputManager& gamepad_manager) const;

        auto& title() const {
            return this->m_title;
        }

        uint32_t width() const;

        uint32_t height() const;

        int32_t xpos() const;

        int32_t ypos() const;

    };

}
