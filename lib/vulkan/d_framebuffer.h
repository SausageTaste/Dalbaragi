#pragma once

#include <vector>

#include "d_vulkan_header.h"
#include "d_image_obj.h"


namespace dal {

    class FbufAttachment {

    public:
        enum class Usage{ color_attachment, depth_attachment, depth_stencil_attachment, depth_map };

    private:
        TextureImage m_image;
        ImageView m_view;
        VkFormat m_format;
        uint32_t m_width = 0, m_height = 0;

    public:
        void init(
            const uint32_t width,
            const uint32_t height,
            const FbufAttachment::Usage usage,
            const VkFormat format,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logiDevice);

        auto& view() const {
            return this->m_view;
        }

        auto format() const {
            return this->m_format;
        }

        auto width() const {
            return this->m_width;
        }

        auto height() const {
            return this->m_height;
        }

        VkExtent2D extent() const {
            return { this->width(), this->height() };
        }

    };


    class AttachmentManager {

    private:
        FbufAttachment m_depth;

        VkExtent2D m_extent{ 0, 0 };

    public:
        void init(
            const VkExtent2D& swapchain_extent,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        auto depth_format() const {
            return this->m_depth.format();
        }

        auto& depth_view() const {
            return this->m_depth.view();
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
