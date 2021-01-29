#pragma once

#include <vector>


namespace dal {

    class VulkanState {

    private:
        class Pimpl;
        Pimpl* m_pimpl = nullptr;

    public:
        ~VulkanState();

        bool init(const char* const window_title, std::vector<const char*> extensions);
        void destroy();

    };

}
