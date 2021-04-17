#include "d_framebuffer.h"

#include <array>
#include <fmt/format.h>

#include "d_logger.h"


namespace {

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

        this->m_depth_image.init_depth(
            swapchain_extent.width,
            swapchain_extent.height,
            phys_device,
            logi_device
        );

        this->m_depth_view.init(
            this->m_depth_image.image(),
            this->m_depth_image.format(),
            1,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            logi_device
        );
    }

    void AttachmentManager::destroy(const VkDevice logi_device) {
        this->m_depth_view.destroy(logi_device);
        this->m_depth_image.destory(logi_device);
    }

}


// FbufManager
namespace dal {

    void FbufManager::init(
        const std::vector<ImageView>& swapchain_views,
        const ImageView& depth_view,
        const VkExtent2D& swapchain_extent,
        const VkRenderPass renderpass,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        this->m_swapchain_fbuf.resize(swapchain_views.size());
        for (uint32_t i = 0; i < swapchain_views.size(); ++i) {
            const std::array<VkImageView, 2> attachments{
                swapchain_views.at(i).get(),
                depth_view.get(),
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
    }

    void FbufManager::destroy(const VkDevice logi_device) {
        for (auto fbuf : this->m_swapchain_fbuf) {
            vkDestroyFramebuffer(logi_device, fbuf, nullptr);
        }
        this->m_swapchain_fbuf.clear();

    }

}
