#pragma once

#include <vector>

#include "d_vulkan_header.h"
#include "d_image_obj.h"


namespace dal {

    class AttachmentManager {

    private:
        TextureImage m_depth_image;
        ImageView m_depth_view;

        VkExtent2D m_extent{ 0, 0 };

    public:
        void init(
            const VkExtent2D& swapchain_extent,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        auto depth_format() const {
            return this->m_depth_image.format();
        }

        auto& depth_view() const {
            return this->m_depth_view;
        }

    };


    class FbufManager {

    private:
        std::vector<VkFramebuffer> m_swapchain_fbuf;

    public:
        void init(
            const std::vector<ImageView>& swapchain_views,
            const ImageView& depth_view,
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
