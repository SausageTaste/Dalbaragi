#include "d_render_pass.h"

#include <array>
#include <cassert>
#include <stdexcept>

#include "d_logger.h"


namespace {

    class AttachmentDescBuilder {

    private:
        std::vector<VkAttachmentDescription> m_attachments;

    public:
        auto data() const {
            return this->m_attachments.data();
        }

        auto size() const {
            return this->m_attachments.size();
        }

        VkAttachmentDescription& add(
            const VkFormat format,
            const VkSampleCountFlagBits samples,
            const VkAttachmentLoadOp load_op,
            const VkAttachmentStoreOp store_op,
            const VkAttachmentLoadOp stencil_load_op,
            const VkAttachmentStoreOp stencil_store_op,
            const VkImageLayout initial_layout,
            const VkImageLayout final_layout
        ) {
            this->m_attachments.emplace_back();

            this->m_attachments.back().format = format;
            this->m_attachments.back().samples = samples;
            this->m_attachments.back().loadOp = load_op;
            this->m_attachments.back().storeOp = store_op;
            this->m_attachments.back().stencilLoadOp = stencil_load_op;
            this->m_attachments.back().stencilStoreOp = stencil_store_op;
            this->m_attachments.back().initialLayout = initial_layout;
            this->m_attachments.back().finalLayout = final_layout;

            return this->m_attachments.back();
        }

        VkAttachmentDescription& add(
            const VkFormat format,
            const VkAttachmentLoadOp load_op,
            const VkAttachmentStoreOp store_op,
            const VkImageLayout initial_layout,
            const VkImageLayout final_layout
        ) {
            return this->add(
                format,
                VK_SAMPLE_COUNT_1_BIT,
                load_op,
                store_op,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                VK_ATTACHMENT_STORE_OP_DONT_CARE,
                initial_layout,
                final_layout
            );
        }

        VkAttachmentDescription& add(const VkFormat format, const VkImageLayout final_layout) {
            return this->add(
                format,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_STORE,
                VK_IMAGE_LAYOUT_UNDEFINED,
                final_layout
            );
        }

    };


    VkRenderPass create_renderpass_gbuf(
        const VkFormat format_color,
        const VkFormat format_depth,
        const VkFormat format_albedo,
        const VkFormat format_materials,
        const VkFormat format_normal,
        const VkDevice logi_device
    ) {
        AttachmentDescBuilder attachments;
        attachments.add(format_color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        attachments.add(format_depth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        attachments.add(format_albedo, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        attachments.add(format_materials, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        attachments.add(format_normal, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        VkAttachmentReference depth_attachment_ref{};
        depth_attachment_ref.attachment = 1;
        depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        std::array<VkSubpassDescription, 2> subpasses{};

        // First subpass
        // ---------------------------------------------------------------------------------

        std::array<VkAttachmentReference, 3> color_attachment_ref{};
        color_attachment_ref[0] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };  // albedo
        color_attachment_ref[1] = { 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };  // materials
        color_attachment_ref[2] = { 4, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };  // normal

        subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        // Index of color attachment can be used with index number if fragment shader
        // like "layout(location = 0) out vec4 outColor".
        subpasses[0].colorAttachmentCount = color_attachment_ref.size();
        subpasses[0].pColorAttachments = color_attachment_ref.data();
        subpasses[0].pDepthStencilAttachment = &depth_attachment_ref;

        // Second subpass
        // ---------------------------------------------------------------------------------

        std::array<VkAttachmentReference, 1> color_attachment_ref_in_2{};
        color_attachment_ref_in_2[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        std::array<VkAttachmentReference, 4> input_references_in_2;
        input_references_in_2[0] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        input_references_in_2[1] = { 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        input_references_in_2[2] = { 3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        input_references_in_2[3] = { 4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

        subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[1].colorAttachmentCount = color_attachment_ref_in_2.size();
        subpasses[1].pColorAttachments = color_attachment_ref_in_2.data();
        subpasses[1].pDepthStencilAttachment = nullptr;
        subpasses[1].inputAttachmentCount = input_references_in_2.size();
        subpasses[1].pInputAttachments = input_references_in_2.data();

        // Dependencies
        // ---------------------------------------------------------------------------------

        std::array<VkSubpassDependency, 3> dependencies{};

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = 1;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[2].srcSubpass = 0;
        dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        // Create render pass
        // ---------------------------------------------------------------------------------

        VkRenderPassCreateInfo renderpass_info{};
        renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderpass_info.attachmentCount = attachments.size();
        renderpass_info.pAttachments    = attachments.data();
        renderpass_info.subpassCount    = subpasses.size();
        renderpass_info.pSubpasses      = subpasses.data();
        renderpass_info.dependencyCount = dependencies.size();
        renderpass_info.pDependencies   = dependencies.data();

        VkRenderPass renderpass = VK_NULL_HANDLE;
        if ( VK_SUCCESS != vkCreateRenderPass(logi_device, &renderpass_info, nullptr, &renderpass) ) {
            dalAbort("failed to create render pass");
        }

        return renderpass;
    }

    VkRenderPass create_renderpass_final(
        const VkFormat format_color,
        const VkDevice logi_device
    ) {
        AttachmentDescBuilder attachments;
        attachments.add(format_color, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        std::array<VkSubpassDescription, 1> subpasses{};

        // First subpass
        // ---------------------------------------------------------------------------------

        std::array<VkAttachmentReference, 1> color_attachment_ref{};
        color_attachment_ref[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[0].colorAttachmentCount = color_attachment_ref.size();
        subpasses[0].pColorAttachments = color_attachment_ref.data();
        subpasses[0].pDepthStencilAttachment = nullptr;

        // Dependencies
        // ---------------------------------------------------------------------------------

        std::array<VkSubpassDependency, 1> dependency{};
        dependency.at(0).srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.at(0).dstSubpass = 0;
        dependency.at(0).srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.at(0).srcAccessMask = 0;
        dependency.at(0).dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.at(0).dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

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

    VkRenderPass create_renderpass_alpha(
        const VkFormat format_color,
        const VkFormat format_depth,
        const VkDevice logi_device
    ) {
        AttachmentDescBuilder attachments;
        attachments.add(
            format_color,
            VK_ATTACHMENT_LOAD_OP_LOAD,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
        attachments.add(
            format_depth,
            VK_ATTACHMENT_LOAD_OP_LOAD,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        );

        VkAttachmentReference depth_attachment_ref{};
        depth_attachment_ref.attachment = 1;
        depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        std::array<VkAttachmentReference, 1> color_attachment_ref{};
        color_attachment_ref[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        std::array<VkSubpassDescription, 1> subpasses{};
        subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[0].colorAttachmentCount = color_attachment_ref.size();
        subpasses[0].pColorAttachments = color_attachment_ref.data();
        subpasses[0].pDepthStencilAttachment = &depth_attachment_ref;

        // Dependencies
        // ---------------------------------------------------------------------------------

        std::array<VkSubpassDependency, 1> dependencies{};

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // Create render pass
        // ---------------------------------------------------------------------------------

        VkRenderPassCreateInfo renderpass_info{};
        renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderpass_info.attachmentCount = attachments.size();
        renderpass_info.pAttachments    = attachments.data();
        renderpass_info.subpassCount    = subpasses.size();
        renderpass_info.pSubpasses      = subpasses.data();
        renderpass_info.dependencyCount = dependencies.size();
        renderpass_info.pDependencies   = dependencies.data();

        VkRenderPass renderpass = VK_NULL_HANDLE;
        if ( VK_SUCCESS != vkCreateRenderPass(logi_device, &renderpass_info, nullptr, &renderpass) ) {
            dalAbort("failed to create render pass");
        }

        return renderpass;
    }

    VkRenderPass create_renderpass_shadow_map(
        const VkFormat format_shadow_map,
        const VkDevice logi_device
    ) {
        AttachmentDescBuilder attachments;
        attachments.add(format_shadow_map, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        VkAttachmentReference depth_attachment_ref{};
        depth_attachment_ref.attachment = 0;
        depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        std::vector<VkSubpassDescription> subpasses;
        subpasses.emplace_back();
        subpasses.back().pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses.back().colorAttachmentCount    = 0;
        subpasses.back().pColorAttachments       = nullptr;
        subpasses.back().pDepthStencilAttachment = &depth_attachment_ref;

        // Dependencies
        // ---------------------------------------------------------------------------------

        std::vector<VkSubpassDependency> dependencies;

        // Create render pass
        // ---------------------------------------------------------------------------------

        VkRenderPassCreateInfo renderpass_info{};
        renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderpass_info.attachmentCount = attachments.size();
        renderpass_info.pAttachments    = attachments.data();
        renderpass_info.subpassCount    = subpasses.size();
        renderpass_info.pSubpasses      = subpasses.data();
        renderpass_info.dependencyCount = dependencies.size();
        renderpass_info.pDependencies   = dependencies.data();

        VkRenderPass renderpass = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateRenderPass(logi_device, &renderpass_info, nullptr, &renderpass))
            dalAbort("failed to create render pass");

        return renderpass;
    }

    VkRenderPass create_renderpass_simple(
        const VkFormat format_color,
        const VkFormat format_depth,
        const VkDevice logi_device
    ) {
        AttachmentDescBuilder attachments;
        attachments.add(format_color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        attachments.add(format_depth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        VkAttachmentReference depth_attachment_ref{};
        depth_attachment_ref.attachment = 1;
        depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        std::array<VkAttachmentReference, 1> color_attachment_ref{};
        color_attachment_ref[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        std::array<VkSubpassDescription, 1> subpasses{};
        subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[0].colorAttachmentCount = color_attachment_ref.size();
        subpasses[0].pColorAttachments = color_attachment_ref.data();
        subpasses[0].pDepthStencilAttachment = &depth_attachment_ref;

        // Dependencies
        // ---------------------------------------------------------------------------------

        std::array<VkSubpassDependency, 1> dependencies{};

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // Create render pass
        // ---------------------------------------------------------------------------------

        VkRenderPassCreateInfo renderpass_info{};
        renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderpass_info.attachmentCount = attachments.size();
        renderpass_info.pAttachments    = attachments.data();
        renderpass_info.subpassCount    = subpasses.size();
        renderpass_info.pSubpasses      = subpasses.data();
        renderpass_info.dependencyCount = dependencies.size();
        renderpass_info.pDependencies   = dependencies.data();

        VkRenderPass renderpass = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateRenderPass(logi_device, &renderpass_info, nullptr, &renderpass))
            dalAbort("failed to create render pass");

        return renderpass;
    }

}


// IRenderPass
namespace dal {

    IRenderPass::IRenderPass(IRenderPass&& other) noexcept {
        std::swap(this->m_handle, other.m_handle);
    }

    IRenderPass& IRenderPass::operator=(IRenderPass&& other) noexcept {
        std::swap(this->m_handle, other.m_handle);
        return *this;
    }

    IRenderPass::~IRenderPass() {
        dalAssert(VK_NULL_HANDLE == this->m_handle);
    }

    void IRenderPass::destroy(const VkDevice logi_device) {
        if (VK_NULL_HANDLE != this->m_handle) {
            vkDestroyRenderPass(logi_device, this->m_handle, nullptr);
            this->m_handle = VK_NULL_HANDLE;
        }
    }

    VkRenderPass IRenderPass::get() const {
        dalAssert(this->is_ready());
        return this->m_handle;
    }

}


// RenderPass implementaions
namespace dal {

    void RenderPass_Gbuf::init(
        const VkFormat format_color,
        const VkFormat format_depth,
        const VkFormat format_albedo,
        const VkFormat format_materials,
        const VkFormat format_normal,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        this->m_format_color = format_color;
        this->m_format_depth = format_depth;
        this->m_format_albedo = format_albedo;
        this->m_format_materials = format_materials;
        this->m_format_normal = format_normal;

        this->m_handle = ::create_renderpass_gbuf(
            format_color,
            format_depth,
            format_albedo,
            format_materials,
            format_normal,
            logi_device
        );
    }

    void RenderPass_Final::init(
        const VkFormat swapchain_img_format,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        this->m_format_swapchain = swapchain_img_format;

        this->m_handle = ::create_renderpass_final(swapchain_img_format, logi_device);
    }

    void RenderPass_Alpha::init(
        const VkFormat format_color,
        const VkFormat format_depth,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        this->m_format_color = format_color;
        this->m_format_depth = format_depth;

        this->m_handle = ::create_renderpass_alpha(format_color, format_depth, logi_device);
    }

    void RenderPass_ShadowMap::init(
        const VkFormat format_shadow_map,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        this->m_format_shadow_map = format_shadow_map;

        this->m_handle = ::create_renderpass_shadow_map(format_shadow_map, logi_device);
    }

    void RenderPass_Simple::init(
        const VkFormat format_color,
        const VkFormat format_depth,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        this->m_format_color = format_color;
        this->m_format_depth = format_depth;

        this->m_handle = ::create_renderpass_simple(
            format_color,
            format_depth,
            logi_device
        );
    }

}


namespace dal {

    void RenderPassManager::init(
        const VkFormat format_swapchain,
        const VkFormat format_depth,
        const VkDevice logi_device
    ) {
        const auto format_color = VK_FORMAT_B10G11R11_UFLOAT_PACK32;
        const auto format_albedo = VK_FORMAT_R8G8B8A8_UNORM;
        const auto format_materials = VK_FORMAT_R8G8B8A8_UNORM;
        const auto format_normal = VK_FORMAT_R8G8B8A8_UNORM;

        this->m_rp_gbuf.init(format_color, format_depth, format_albedo, format_materials, format_normal, logi_device);
        this->m_rp_final.init(format_swapchain, logi_device);
        this->m_rp_alpha.init(format_color, format_depth, logi_device);
        this->m_rp_shadow.init(format_depth, logi_device);
        this->m_rp_simple.init(format_color, format_depth, logi_device);
    }

    void RenderPassManager::destroy(const VkDevice logi_device) {
        this->m_rp_gbuf.destroy(logi_device);
        this->m_rp_final.destroy(logi_device);
        this->m_rp_alpha.destroy(logi_device);
        this->m_rp_shadow.destroy(logi_device);
        this->m_rp_simple.destroy(logi_device);
    }

}
