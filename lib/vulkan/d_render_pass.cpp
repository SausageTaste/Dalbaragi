#include "d_render_pass.h"

#include <array>
#include <cassert>
#include <stdexcept>

#include "d_logger.h"


namespace {

    VkRenderPass create_renderpass_rendering(const std::array<VkFormat, 1>& attachment_formats, const VkDevice logi_device) {
        std::array<VkAttachmentDescription, 1> attachments{};
        {
            attachments[0].format = attachment_formats.at(0);
            attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }

        std::array<VkSubpassDescription, 1> subpasses{};

        // First subpass
        // ---------------------------------------------------------------------------------

        std::array<VkAttachmentReference, 1> color_attachment_ref{};
        color_attachment_ref[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        // Index of color attachment can be used with index number if fragment shader
        // like "layout(location = 0) out vec4 outColor".
        subpasses[0].colorAttachmentCount = color_attachment_ref.size();
        subpasses[0].pColorAttachments = color_attachment_ref.data();

        // Dependencies
        // ---------------------------------------------------------------------------------

        std::array<VkSubpassDependency, 1> dependency{};
        dependency.at(0).srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.at(0).dstSubpass = 0;
        dependency.at(0).srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.at(0).srcAccessMask = 0;
        dependency.at(0).dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.at(0).dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // Create render pass
        // ---------------------------------------------------------------------------------

        VkRenderPassCreateInfo renderpass_info{};
        renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderpass_info.attachmentCount = attachments.size();
        renderpass_info.pAttachments    = attachments.data();
        renderpass_info.subpassCount    = subpasses.size();
        renderpass_info.pSubpasses      = subpasses.data();
        renderpass_info.dependencyCount = dependency.size();
        renderpass_info.pDependencies   = dependency.data();

        VkRenderPass renderpass = VK_NULL_HANDLE;
        if ( VK_SUCCESS != vkCreateRenderPass(logi_device, &renderpass_info, nullptr, &renderpass) ) {
            dalAbort("failed to create render pass");
        }

        return renderpass;
    }

}


// RenderPass
namespace dal {

    RenderPass::~RenderPass() {
        dalAssert(VK_NULL_HANDLE == this->m_handle);
    }

    void RenderPass::destroy(const VkDevice logi_device) {
        if (VK_NULL_HANDLE != this->m_handle) {
            vkDestroyRenderPass(logi_device, this->m_handle, nullptr);
            this->m_handle = VK_NULL_HANDLE;
        }
    }

}


namespace dal {

    void RenderPassManager::init(const std::array<VkFormat, 1>& attachment_formats, const VkDevice logi_device) {
        this->destroy(logi_device);

        this->m_rp_rendering = ::create_renderpass_rendering(attachment_formats, logi_device);
    }

    void RenderPassManager::destroy(VkDevice logi_device) {
        this->m_rp_rendering.destroy(logi_device);
    }

}
