#pragma once

#include <vector>

#include "d_vulkan_header.h"
#include "d_image_obj.h"


namespace dal {

    class FbufManager {

    private:
        std::vector<VkFramebuffer> m_swapchain_fbuf;

    public:
        void init(
            const std::vector<ImageView>& swapchain_views,
            const VkExtent2D& swapchain_extent,
            const VkRenderPass renderpass,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        auto& swapchain_fbuf() const {
            return this->m_swapchain_fbuf;
        }

    };

}
