#pragma once

#include <vector>

#include "d_render_pass.h"
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


    class Framebuffer {

    private:
        VkFramebuffer m_handle = VK_NULL_HANDLE;

    public:
        Framebuffer() = default;
        Framebuffer(const Framebuffer&) = delete;
        Framebuffer& operator=(const Framebuffer&) = delete;

    public:
        Framebuffer(Framebuffer&&) noexcept;

        Framebuffer& operator=(Framebuffer&&) noexcept;

        ~Framebuffer();

        [[nodiscard]]
        bool create(
            const VkImageView* const attachments,
            const uint32_t attachment_count,
            const uint32_t width,
            const uint32_t height,
            const VkRenderPass renderpass,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        bool is_ready() const {
            return VK_NULL_HANDLE != this->m_handle;
        }

        VkFramebuffer get() const {
            return this->m_handle;
        }

    };


    class Fbuf_Simple : public Framebuffer {

    public:
        void init(
            const dal::RenderPass_Gbuf& rp_gbuf,
            const VkExtent2D& swapchain_extent,
            const VkImageView swapchain_view,
            const VkImageView depth_view,
            const VkDevice logi_device
        );

    };

}
