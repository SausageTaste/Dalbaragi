#pragma once

#include <string>
#include <vector>
#include <functional>

#include "d_inputs.h"


namespace dal {

    class WindowGLFW {

    private:
        void* m_window = nullptr;
        std::string m_title;

    public:
        WindowGLFW(const char* const title);
        ~WindowGLFW();

        void do_frame();
        bool should_close() const;
        std::vector<const char*> get_vulkan_extensions() const;
        std::function<void*(void*)> get_vk_surface_creator() const;

        void set_callback_fbuf_resize(std::function<void(int, int)> func);
        void set_callback_mouse_event(std::function<void(const dal::MouseEvent&)> func);
        void set_callback_key_event(std::function<void(const dal::KeyEvent&)> func);

        auto& title() const {
            return this->m_title;
        }
        uint32_t width() const;
        uint32_t height() const;

    };

}
