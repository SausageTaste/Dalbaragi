#pragma once

#include "d_vulkan_header.h"


namespace dal {

    class SwapchainManager {

    private:
        VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;

        VkFormat m_imageFormat;
        VkExtent2D m_extent;

    public:
        ~SwapchainManager();

        void init();
        void destroy();

    };

}
