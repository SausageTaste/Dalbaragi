#include "d_shader.h"

#include <array>

#include "d_logger.h"
#include "d_filesystem.h"
#include "d_vert_data.h"


// Shader module tools
namespace {

    class ShaderModule {

    private:
        const VkDevice m_parent_device;
        VkShaderModule m_module = VK_NULL_HANDLE;

    public:
        ShaderModule(const VkDevice logi_device)
            : m_parent_device(logi_device)
        {

        }

        ShaderModule(const VkDevice logi_device, const char* const source_str, const uint32_t source_size)
            : m_parent_device(logi_device)
        {
            this->init(source_str, source_size);
        }

        ~ShaderModule() {
            this->destroy();
        }

        void init(const char* const source_str, const uint32_t source_size) {
            this->destroy();

            VkShaderModuleCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            create_info.codeSize = source_size;
            create_info.pCode = reinterpret_cast<const uint32_t*>(source_str);

            if (VK_SUCCESS != vkCreateShaderModule(this->m_parent_device, &create_info, nullptr, &this->m_module)) {
                dalAbort("Failed to create shader module");
            }
        }

        void destroy() {
            if (VK_NULL_HANDLE != this->m_module) {
                vkDestroyShaderModule(this->m_parent_device, this->m_module, nullptr);
                this->m_module = VK_NULL_HANDLE;
            }
        }

        VkShaderModule get() const {
            return this->m_module;
        }

        VkShaderModule operator*() const {
            return this->get();
        }

    };

}


// ShaderPipeline
namespace dal {

    ShaderPipeline::ShaderPipeline(ShaderPipeline&& other) noexcept {
        std::swap(this->m_pipeline, other.m_pipeline);
        std::swap(this->m_layout, other.m_layout);
    }

    ShaderPipeline& ShaderPipeline::operator=(ShaderPipeline&& other) noexcept {
        std::swap(this->m_pipeline, other.m_pipeline);
        std::swap(this->m_layout, other.m_layout);

        return *this;
    }

    void ShaderPipeline::init(const VkPipeline pipeline, const VkPipelineLayout layout, const VkDevice logi_device) {
        this->destroy(logi_device);

        this->m_pipeline = pipeline;
        this->m_layout = layout;
    }

    void ShaderPipeline::destroy(const VkDevice logi_device) {
        if (VK_NULL_HANDLE != this->m_layout) {
            vkDestroyPipelineLayout(logi_device, this->m_layout, nullptr);
            this->m_layout = VK_NULL_HANDLE;
        }

        if (VK_NULL_HANDLE != this->m_pipeline) {
            vkDestroyPipeline(logi_device, this->m_pipeline, nullptr);
            this->m_pipeline = VK_NULL_HANDLE;
        }
    }

}


// Pipeline creating functions
namespace {

    auto create_info_shader_stage(const ShaderModule& vert_shader_module, const ShaderModule& frag_shader_module) {
        std::array<VkPipelineShaderStageCreateInfo, 2> result{};

        result[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        result[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        result[0].module = vert_shader_module.get();
        result[0].pName = "main";

        result[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        result[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        result[1].module = frag_shader_module.get();
        result[1].pName = "main";

        return result;
    }

    auto create_vertex_input_state(
        const   VkVertexInputBindingDescription* const binding_descriptions, const uint32_t binding_count,
        const VkVertexInputAttributeDescription* const attrib_descriptions,  const uint32_t attrib_count
    ) {
        VkPipelineVertexInputStateCreateInfo vertex_input_state{};
        vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        vertex_input_state.vertexBindingDescriptionCount = binding_count;
        vertex_input_state.pVertexBindingDescriptions = binding_descriptions;
        vertex_input_state.vertexAttributeDescriptionCount = attrib_count;
        vertex_input_state.pVertexAttributeDescriptions = attrib_descriptions;

        return vertex_input_state;
    }

    auto create_info_input_assembly() {
        VkPipelineInputAssemblyStateCreateInfo input_assembly{};
        input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

        input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly.primitiveRestartEnable = VK_FALSE;

        return input_assembly;
    }

    auto create_info_viewport_scissor(const VkExtent2D& extent) {
        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0;
        viewport.maxDepth = 1;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;

        return std::make_pair(viewport, scissor);
    }

    auto create_info_viewport_state(
        const VkViewport* const viewports, const uint32_t viewport_count,
        const   VkRect2D* const scissors,  const uint32_t scissor_count
    ) {
        VkPipelineViewportStateCreateInfo viewport_state{};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

        viewport_state.viewportCount = viewport_count;
        viewport_state.pViewports = viewports;
        viewport_state.scissorCount = scissor_count;
        viewport_state.pScissors = scissors;

        return viewport_state;
    }

    auto create_info_rasterizer(
        const VkCullModeFlags cull_mode,
        const bool enable_bias,
        const float bias_constant,
        const float bias_slope
    ) {
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

        rasterizer.depthClampEnable = VK_FALSE;  // vulkan-tutorial.com said this requires GPU feature enabled.

        // Discards all fragents. But why would you ever want it? Well, check the link below.
        // https://stackoverflow.com/questions/42470669/when-does-it-make-sense-to-turn-off-the-rasterization-step
        rasterizer.rasterizerDiscardEnable = VK_FALSE;

        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;  // Any mode other than FILL requires GPU feature enabled.
        rasterizer.lineWidth = 1;  // GPU feature, `wideLines` required for lines thicker than 1.
        rasterizer.cullMode = cull_mode;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = enable_bias;  // Maybe this is used to deal with shadow acne?
        rasterizer.depthBiasConstantFactor = bias_constant;
        rasterizer.depthBiasSlopeFactor = bias_slope;
        rasterizer.depthBiasClamp = 0;

        return rasterizer;
    }

    auto create_info_multisampling() {
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        return multisampling;
    }

    template <size_t _Size, bool _AlphaBlend>
    auto create_info_color_blend_attachment() {
        VkPipelineColorBlendAttachmentState attachment_state_template{};

        attachment_state_template.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        attachment_state_template.colorBlendOp = VK_BLEND_OP_ADD;
        attachment_state_template.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        attachment_state_template.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        attachment_state_template.alphaBlendOp = VK_BLEND_OP_ADD;

        if constexpr (_AlphaBlend) {
            attachment_state_template.blendEnable = VK_TRUE;
            attachment_state_template.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            attachment_state_template.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        }
        else {
            attachment_state_template.blendEnable = VK_FALSE;
            attachment_state_template.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            attachment_state_template.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        }

        std::array<VkPipelineColorBlendAttachmentState, _Size> result;

        for (auto& x : result) {
            x = attachment_state_template;
        }

        return result;
    }

    auto create_info_color_blend(
        const VkPipelineColorBlendAttachmentState* const color_blend_attachments,
        const uint32_t attachment_count,
        const bool enable_logic_op
    ) {
        VkPipelineColorBlendStateCreateInfo color_blending{};
        color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

        color_blending.logicOpEnable = enable_logic_op ? VK_TRUE : VK_FALSE;
        color_blending.logicOp = VK_LOGIC_OP_COPY;
        color_blending.attachmentCount = attachment_count;
        color_blending.pAttachments = color_blend_attachments;
        color_blending.blendConstants[0] = 0;
        color_blending.blendConstants[1] = 0;
        color_blending.blendConstants[2] = 0;
        color_blending.blendConstants[3] = 0;

        return color_blending;
    }

    auto create_info_depth_stencil(const bool depth_write) {
        VkPipelineDepthStencilStateCreateInfo depth_stencil{};
        depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        depth_stencil.depthTestEnable = VK_TRUE;
        depth_stencil.depthWriteEnable = depth_write ? VK_TRUE : VK_FALSE;
        depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depth_stencil.depthBoundsTestEnable = VK_FALSE;
        depth_stencil.minDepthBounds = 0;
        depth_stencil.maxDepthBounds = 1;
        depth_stencil.stencilTestEnable = VK_FALSE;
        depth_stencil.front = {};
        depth_stencil.back = {};

        return depth_stencil;
    }

    auto create_info_dynamic_state(const VkDynamicState* const dynamic_states, const uint32_t dynamic_count) {
        VkPipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

        dynamic_state.dynamicStateCount = dynamic_count;
        dynamic_state.pDynamicStates = dynamic_states;

        return dynamic_state;
    }

    template <typename _Struct>
    auto create_info_push_constant() {
        std::array<VkPushConstantRange, 1> result;

        result[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        result[0].offset = 0;
        result[0].size = sizeof(_Struct);

        return result;
    }

    auto create_pipeline_layout(
        const VkDescriptorSetLayout* const layouts,     const uint32_t layout_count,
        const   VkPushConstantRange* const push_consts, const uint32_t push_const_count,
        const VkDevice logi_device
    ) {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        pipelineLayoutInfo.setLayoutCount = layout_count;
        pipelineLayoutInfo.pSetLayouts = layouts;
        pipelineLayoutInfo.pushConstantRangeCount = push_const_count;
        pipelineLayoutInfo.pPushConstantRanges = push_consts;

        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        if ( vkCreatePipelineLayout(logi_device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS ) {
            dalAbort("failed to create pipeline layout!");
        }

        return pipelineLayout;
    }


    dal::ShaderPipeline make_pipeline_simple(
        dal::filesystem::AssetManager& asset_mgr,
        const bool need_gamma_correction,
        const VkExtent2D& swapchain_extent,
        const VkDescriptorSetLayout desc_layout_simple,
        const VkDescriptorSetLayout desc_layout_per_material,
        const VkDescriptorSetLayout desc_layout_per_actor,
        const VkRenderPass renderpass,
        const VkDevice logi_device
    ) {
        const auto vert_src = asset_mgr.open("spv/simple_v.spv")->read_stl<std::vector<char>>();
        if (!vert_src) {
            dalAbort("Vertex shader 'simple_v.spv' not found");
        }
        const auto frag_src = need_gamma_correction ?
            asset_mgr.open("spv/simple_gamma_f.spv")->read_stl<std::vector<char>>() :
            asset_mgr.open("spv/simple_f.spv")->read_stl<std::vector<char>>();
        if (!frag_src) {
            dalAbort("Fragment shader 'simple_f.spv' not found");
        }

        // Shaders
        const ShaderModule vert_shader_module(logi_device, vert_src->data(), vert_src->size());
        const ShaderModule frag_shader_module(logi_device, frag_src->data(), frag_src->size());
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = ::create_info_shader_stage(vert_shader_module, frag_shader_module);

        // Vertex input state
        const auto binding_desc = dal::make_vert_binding_desc();
        const auto attrib_desc = dal::make_vert_attribute_descriptions();
        auto vertex_input_state = ::create_vertex_input_state(&binding_desc, 1, attrib_desc.data(), attrib_desc.size());

        // Input assembly
        const VkPipelineInputAssemblyStateCreateInfo input_assembly = ::create_info_input_assembly();

        // Viewports and scissors
        const auto [viewport, scissor] = ::create_info_viewport_scissor(swapchain_extent);
        const auto viewport_state = ::create_info_viewport_state(&viewport, 1, &scissor, 1);

        // Rasterizer
        const auto rasterizer = ::create_info_rasterizer(VK_CULL_MODE_BACK_BIT, false, 0, 0);

        // Multisampling
        const auto multisampling = ::create_info_multisampling();

        // Color blending
        const auto color_blend_attachments = ::create_info_color_blend_attachment<1, false>();
        const auto color_blending = ::create_info_color_blend(color_blend_attachments.data(), color_blend_attachments.size(), false);

        // Depth, stencil
        const auto depth_stencil = ::create_info_depth_stencil(true);

        // Dynamic state
        //constexpr std::array<VkDynamicState, 0> dynamic_states{};
        //const auto dynamic_state_info = ::create_info_dynamic_state(dynamic_states.data(), dynamic_states.size());

        // Pipeline layout
        const std::array<VkDescriptorSetLayout, 3> desc_layouts{ desc_layout_simple, desc_layout_per_material, desc_layout_per_actor };
        const auto pipeline_layout = ::create_pipeline_layout(desc_layouts.data(), desc_layouts.size(), nullptr, 0, logi_device);

        // Pipeline, finally
        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = shaderStages.size();
        pipeline_info.pStages = shaderStages.data();
        pipeline_info.pVertexInputState = &vertex_input_state;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pDepthStencilState = &depth_stencil;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = nullptr;
        pipeline_info.layout = pipeline_layout;
        pipeline_info.renderPass = renderpass;
        pipeline_info.subpass = 0;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_info.basePipelineIndex = -1;

        VkPipeline graphics_pipeline;
        if (vkCreateGraphicsPipelines(logi_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline) != VK_SUCCESS) {
            dalAbort("failed to create graphics pipeline!");
        }

        return dal::ShaderPipeline{ graphics_pipeline, pipeline_layout, logi_device };
    }

}


// PipelineManager
namespace dal {

    void PipelineManager::init(
        dal::filesystem::AssetManager& asset_mgr,
        const bool need_gamma_correction,
        const VkExtent2D& swapchain_extent,
        const VkDescriptorSetLayout desc_layout_simple,
        const VkDescriptorSetLayout desc_layout_per_material,
        const VkDescriptorSetLayout desc_layout_per_actor,
        const VkRenderPass renderpass,
        const VkDevice logi_device
    ) {
        this->destroy(logi_device);

        this->m_simple = ::make_pipeline_simple(
            asset_mgr,
            need_gamma_correction,
            swapchain_extent,
            desc_layout_simple,
            desc_layout_per_material,
            desc_layout_per_actor,
            renderpass,
            logi_device
        );
    }

    void PipelineManager::destroy(const VkDevice logi_device) {
        this->m_simple.destroy(logi_device);
    }

}
