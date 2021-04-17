#include "d_framebuffer.h"

#include <array>
#include <fmt/format.h>

#include "d_logger.h"


namespace {

    VkFormat find_supported_format(
        const std::initializer_list<VkFormat>& candidates,
        const VkImageTiling tiling,
        const VkFormatFeatureFlags features,
        const VkPhysicalDevice phys_device
    ) {
        for (const VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(phys_device, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        dalAbort("failed to find supported format!");
    }

    VkFormat find_depth_format(const VkPhysicalDevice phys_device) {
        return ::find_supported_format(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
            phys_device
        );
    }

    auto interpret_usage(const dal::FbufAttachment::Usage usage) {
        VkImageAspectFlags aspect_mask;
        VkImageLayout image_layout;
        VkImageUsageFlags flag;

        switch (usage) {
            case dal::FbufAttachment::Usage::color_attachment:
                aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
                image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                flag = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
                break;

            case dal::FbufAttachment::Usage::depth_map:
                aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
                image_layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                flag =  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                break;

            case dal::FbufAttachment::Usage::depth_stencil_attachment:
                aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
                image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                flag = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
                break;
        }

        return std::make_tuple(flag, aspect_mask, image_layout);
    }

}


// ColorAttachment
namespace dal {

    void FbufAttachment::init(
        const uint32_t width,
        const uint32_t height,
        const FbufAttachment::Usage usage,
        const VkFormat format,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        this->m_format = format;
        this->m_width = width;
        this->m_height = height;
        const auto [usage_flag, aspect_mask, image_layout] = ::interpret_usage(usage);

        this->m_image.init_attachment(
            this->width(),
            this->height(),
            this->format(),
            usage_flag,
            phys_device,
            logi_device
        );

        const auto view_init_result = this->m_view.init(
            this->m_image.image(),
            this->format(),
            1,
            aspect_mask,
            logi_device
        );
        dalAssert(view_init_result);
    }

    void FbufAttachment::destroy(const VkDevice logiDevice) {
        this->m_image.destory(logiDevice);
        this->m_view.destroy(logiDevice);
    }

}


// AttachmentManager
namespace dal {

    void AttachmentManager::init(
        const VkExtent2D& swapchain_extent,
        const VkPhysicalDevice phys_device,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        this->m_extent = swapchain_extent;

        this->m_depth.init(
            this->m_extent.width,
            this->m_extent.height,
            dal::FbufAttachment::Usage::depth_stencil_attachment,
            ::find_depth_format(phys_device),
            phys_device,
            logi_device
        );

        this->m_color.init(
            this->m_extent.width,
            this->m_extent.height,
            dal::FbufAttachment::Usage::color_attachment,
            VK_FORMAT_R8G8B8A8_UNORM,
            phys_device,
            logi_device
        );
    }

    void AttachmentManager::destroy(const VkDevice logi_device) {
        this->m_color.destroy(logi_device);
        this->m_depth.destroy(logi_device);
    }

}


// FbufManager
namespace dal {

    void FbufManager::init(
        const std::vector<ImageView>& swapchain_views,
        const AttachmentManager& attachment_man,
        const VkExtent2D& swapchain_extent,
        const VkRenderPass renderpass,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        this->m_swapchain_fbuf.resize(swapchain_views.size());
        for (uint32_t i = 0; i < swapchain_views.size(); ++i) {
            const std::array<VkImageView, 2> attachments{
                attachment_man.color().view().get(),
                attachment_man.depth().view().get(),
            };

            VkFramebufferCreateInfo fbuf_info{};
            fbuf_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbuf_info.renderPass = renderpass;
            fbuf_info.attachmentCount = attachments.size();
            fbuf_info.pAttachments = attachments.data();
            fbuf_info.width = swapchain_extent.width;
            fbuf_info.height = swapchain_extent.height;
            fbuf_info.layers = 1;

            if (VK_SUCCESS != vkCreateFramebuffer( logi_device, &fbuf_info, nullptr, &this->m_swapchain_fbuf.at(i) )) {
                dalAbort("failed to create framebuffer!");
            }
        }

        {
            const std::array<VkImageView, 2> attachments{
                attachment_man.color().view().get(),
                attachment_man.depth().view().get(),
            };

            VkFramebufferCreateInfo fbuf_info{};
            fbuf_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbuf_info.renderPass = renderpass;
            fbuf_info.attachmentCount = attachments.size();
            fbuf_info.pAttachments = attachments.data();
            fbuf_info.width = swapchain_extent.width;
            fbuf_info.height = swapchain_extent.height;
            fbuf_info.layers = 1;

            if (VK_SUCCESS != vkCreateFramebuffer( logi_device, &fbuf_info, nullptr, &this->m_color_fbuf)) {
                dalAbort("failed to create framebuffer!");
            }
        }
    }

    void FbufManager::destroy(const VkDevice logi_device) {
        for (auto fbuf : this->m_swapchain_fbuf) {
            vkDestroyFramebuffer(logi_device, fbuf, nullptr);
        }
        this->m_swapchain_fbuf.clear();

        if (VK_NULL_HANDLE != this->m_color_fbuf) {
            vkDestroyFramebuffer(logi_device, this->m_color_fbuf, nullptr);
            this->m_color_fbuf = VK_NULL_HANDLE;
        }
    }

}
