#include "d_framebuffer.h"

#include <array>
#include <fmt/format.h>

#include "d_logger.h"


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
