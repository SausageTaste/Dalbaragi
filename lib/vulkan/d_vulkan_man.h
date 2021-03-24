#pragma once

#include <vector>
#include <functional>


namespace dal {

    class VulkanState {

    private:
        class Pimpl;
        Pimpl* m_pimpl = nullptr;

    public:
        VulkanState() = default;
        ~VulkanState();

        VulkanState(
            const char* const window_title,
            const unsigned init_width,
            const unsigned init_height,
            const std::vector<const char*>& extensions,
            std::function<void*(void*)> surface_create_func
        ) {
            this->init(window_title, init_width, init_height, extensions, surface_create_func);
        }

        void init(
            const char* const window_title,
            const unsigned init_width,
            const unsigned init_height,
            const std::vector<const char*>& extensions,
            std::function<void*(void*)> surface_create_func
        );

        void destroy();

        void update();

        void wait_device_idle() const;

    };

}
