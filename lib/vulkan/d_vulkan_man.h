#pragma once

#include <vector>
#include <functional>


namespace dal {

    class VulkanState {

    private:
        class Pimpl;
        Pimpl* m_pimpl = nullptr;

    public:
        ~VulkanState();

        void init(
            const char* const window_title,
            const std::vector<const char*>& extensions,
            std::function<void*(void*)> surface_create_func
        );
        void destroy();

    };

}
