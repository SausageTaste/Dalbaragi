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


    class AttachmentBundle_Simple {

    private:
        FbufAttachment m_color;
        FbufAttachment m_depth;

    public:
        void init(
            const uint32_t width,
            const uint32_t height,
            const dal::RenderPass_Simple& renderpass,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void destroy(const VkDevice logi_device);

        auto& color() const {
            return this->m_color;
        }

        auto& depth() const {
            return this->m_depth;
        }

        auto extent() const {
            return this->color().extent();
        }

    };


    class AttachmentBundle_Gbuf {

    private:
        FbufAttachment m_color;
        FbufAttachment m_depth;
        FbufAttachment m_albedo;
        FbufAttachment m_materials;
        FbufAttachment m_normal;

        VkExtent2D m_extent{ 0, 0 };

    public:
        void init(
            const VkExtent2D& extent,
            const dal::RenderPass_Gbuf& renderpass,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        );

        void init(
            const uint32_t width,
            const uint32_t height,
            const dal::RenderPass_Gbuf& renderpass,
            const VkPhysicalDevice phys_device,
            const VkDevice logi_device
        ) {
            this->init(VkExtent2D{ width, height }, renderpass, phys_device, logi_device);
        }

        void destroy(const VkDevice logi_device);

        auto& color() const {
            return this->m_color;
        }

        auto& depth() const {
            return this->m_depth;
        }

        auto& albedo() const {
            return this->m_albedo;
        }

        auto& materials() const {
            return this->m_materials;
        }

        auto& normal() const {
            return this->m_normal;
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

    protected:
        [[nodiscard]]
        bool create(
            const VkImageView* const attachments,
            const uint32_t attachment_count,
            const uint32_t width,
            const uint32_t height,
            const VkRenderPass renderpass,
            const VkDevice logi_device
        );

    public:
        void destroy(const VkDevice logi_device);

        bool is_ready() const {
            return VK_NULL_HANDLE != this->m_handle;
        }

        VkFramebuffer get() const {
            return this->m_handle;
        }

    };


    class Fbuf_Gbuf : public Framebuffer {

    public:
        void init(
            const dal::RenderPass_Gbuf& renderpass,
            const VkExtent2D& swapchain_extent,
            const VkImageView color_view,
            const VkImageView depth_view,
            const VkImageView albedo_view,
            const VkImageView materials_view,
            const VkImageView normal_view,
            const VkDevice logi_device
        );

    };

    class Fbuf_Final : public Framebuffer {

    public:
        void init(
            const dal::RenderPass_Final& renderpass,
            const VkExtent2D& swapchain_extent,
            const VkImageView swapchain_view,
            const VkDevice logi_device
        );

    };

    class Fbuf_Alpha : public Framebuffer {

    public:
        void init(
            const dal::RenderPass_Alpha& renderpass,
            const VkExtent2D& extent,
            const VkImageView color_view,
            const VkImageView depth_view,
            const VkDevice logi_device
        );

    };

    class Fbuf_Shadow : public Framebuffer {

    public:
        void init(
            const dal::RenderPass_ShadowMap& renderpass,
            const VkExtent2D& extent,
            const VkImageView shadow_map_view,
            const VkDevice logi_device
        );

    };

    class Fbuf_Simple : public Framebuffer {

    public:
        void init(
            const dal::RenderPass_Simple& renderpass,
            const VkExtent2D& extent,
            const VkImageView color_view,
            const VkImageView depth_view,
            const VkDevice logi_device
        );

    };

}
