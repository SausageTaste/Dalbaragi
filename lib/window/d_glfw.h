#pragma once

#include <string>
#include <vector>


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

        auto& title() const {
            return this->m_title;
        }

    };

}
